#pragma once
#include <brpc/server.h>

#include "base.pb.h"
#include "channel.hpp"
#include "data_mysql_user.hpp"
#include "data_redis.hpp"
#include "data_search.hpp"
#include "etcd.hpp"
#include "file.pb.h"
#include "sms.hpp"
#include "user.pb.h"
#include "utils.hpp"

namespace huzch {

class UserServiceImpl : public UserService {
 public:
  UserServiceImpl(const std::shared_ptr<elasticlient::Client>& es_client,
                  const std::shared_ptr<odb::core::database>& mysql_client,
                  const std::shared_ptr<sw::redis::Redis>& redis_client,
                  const SMSClient::Ptr& sms_client,
                  const std::string& file_service_name,
                  const ChannelManager::Ptr& channels)
      : _es_user(std::make_shared<ESUser>(es_client)),
        _mysql_user(std::make_shared<UserTable>(mysql_client)),
        _redis_session(std::make_shared<Session>(redis_client)),
        _redis_status(std::make_shared<Status>(redis_client)),
        _redis_code(std::make_shared<Code>(redis_client)),
        _sms_client(sms_client),
        _file_service_name(file_service_name),
        _channels(channels) {
    _es_user->index();
  }

  void UserRegister(google::protobuf::RpcController* controller,
                    const UserRegisterReq* request, UserRegisterRsp* response,
                    google::protobuf::Closure* done) {
    brpc::ClosureGuard rpc_guard(done);
    std::string request_id = request->request_id();
    response->set_request_id(request_id);

    auto err_rsp = [response](const std::string& errmsg) {
      response->set_success(false);
      response->set_errmsg(errmsg);
    };
    std::string name = request->user_name();
    std::string password = request->password();

    if (!check_name(name)) {
      LOG_ERROR("{} 用户名长度不合法: {}", request_id, name);
      err_rsp("用户名长度不合法");
      return;
    }

    if (!check_password(password)) {
      LOG_ERROR("{} 密码格式不合法: {}", request_id, password);
      err_rsp("密码格式不合法");
      return;
    }

    auto user = _mysql_user->select_by_name(name);
    if (user) {
      LOG_ERROR("{} 用户名已存在: {}", request_id, name);
      err_rsp("用户名已存在");
      return;
    }

    std::string user_id = uuid();
    user = std::make_shared<User>(user_id, name, password);
    bool ret = _mysql_user->insert(user);
    if (!ret) {
      LOG_ERROR("{} mysql新增用户失败", request_id);
      err_rsp("mysql新增用户失败");
      return;
    }

    ret = _es_user->insert(user_id, "", name, "", "");
    if (!ret) {
      LOG_ERROR("{} es新增用户失败", request_id);
      err_rsp("es新增用户失败");
      return;
    }

    response->set_success(true);
  }

  void UserLogin(google::protobuf::RpcController* controller,
                 const UserLoginReq* request, UserLoginRsp* response,
                 google::protobuf::Closure* done) {
    brpc::ClosureGuard rpc_guard(done);
    std::string request_id = request->request_id();
    response->set_request_id(request_id);

    auto err_rsp = [response](const std::string& errmsg) {
      response->set_success(false);
      response->set_errmsg(errmsg);
    };
    std::string name = request->user_name();
    std::string password = request->password();

    auto user = _mysql_user->select_by_name(name);
    if (!user || password != user->password()) {
      LOG_ERROR("{} 用户名或密码错误: {} {} {}", request_id, name, password);
      err_rsp("用户名或密码错误");
      return;
    }

    bool ret = _redis_status->exists(user->user_id());
    if (ret) {
      LOG_ERROR("{} 用户已在其他地方登录: {}", request_id, name);
      err_rsp("用户已在其他地方登录");
      return;
    }

    std::string login_session_id = uuid();
    ret = _redis_session->insert(login_session_id, user->user_id());
    if (!ret) {
      LOG_ERROR("{} redis新增用户会话失败", request_id);
      err_rsp("redis新增用户会话失败");
      return;
    }

    ret = _redis_status->insert(user->user_id());
    if (!ret) {
      LOG_ERROR("{} redis新增用户状态失败", request_id);
      err_rsp("redis新增用户状态失败");
      return;
    }

    response->set_success(true);
    response->set_login_session_id(login_session_id);
  }

  void GetPhoneVerifyCode(google::protobuf::RpcController* controller,
                          const GetPhoneVerifyCodeReq* request,
                          GetPhoneVerifyCodeRsp* response,
                          google::protobuf::Closure* done) {
    brpc::ClosureGuard rpc_guard(done);
    std::string request_id = request->request_id();
    response->set_request_id(request_id);

    auto err_rsp = [response](const std::string& errmsg) {
      response->set_success(false);
      response->set_errmsg(errmsg);
    };
    std::string phone = request->phone_number();

    if (!check_phone(phone)) {
      LOG_ERROR("{} 手机号格式不合法: {}", request_id, phone);
      err_rsp("手机号格式不合法");
      return;
    }

    std::string code_id = uuid();
    std::string code = verify_code();
    bool ret = _sms_client->send(phone, code);
    if (!ret) {
      LOG_ERROR("{} 短信验证码发送失败", request_id);
      err_rsp("短信验证码发送失败");
      return;
    }

    ret = _redis_code->insert(code_id, code);
    if (!ret) {
      LOG_ERROR("{} redis新增用户验证码失败", request_id);
      err_rsp("redis新增用户验证码失败");
      return;
    }

    response->set_success(true);
    response->set_verify_code_id(code_id);
  }

  void PhoneRegister(google::protobuf::RpcController* controller,
                     const PhoneRegisterReq* request,
                     PhoneRegisterRsp* response,
                     google::protobuf::Closure* done) {
    brpc::ClosureGuard rpc_guard(done);
    std::string request_id = request->request_id();
    response->set_request_id(request_id);

    auto err_rsp = [response](const std::string& errmsg) {
      response->set_success(false);
      response->set_errmsg(errmsg);
    };
    std::string phone = request->phone_number();
    std::string code_id = request->verify_code_id();
    std::string code = request->verify_code();

    if (!check_phone(phone)) {
      LOG_ERROR("{} 手机号格式不合法: {}", request_id, phone);
      err_rsp("手机号格式不合法");
      return;
    }

    auto verify_code = _redis_code->code(code_id);
    if (code != verify_code) {
      LOG_ERROR("{} 验证码错误: {}", request_id, code);
      err_rsp("验证码错误");
      return;
    }
    _redis_code->remove(code_id);

    auto user = _mysql_user->select_by_phone(phone);
    if (user) {
      LOG_ERROR("{} 手机号已存在: {}", request_id, phone);
      err_rsp("手机号已存在");
      return;
    }

    std::string user_id = uuid();
    user = std::make_shared<User>(user_id, phone);
    bool ret = _mysql_user->insert(user);
    if (!ret) {
      LOG_ERROR("{} mysql新增用户失败", request_id);
      err_rsp("mysql新增用户失败");
      return;
    }

    ret = _es_user->insert(user_id, "", "", phone, "");
    if (!ret) {
      LOG_ERROR("{} es新增用户失败", request_id);
      err_rsp("es新增用户失败");
      return;
    }

    response->set_success(true);
  }

  void PhoneLogin(google::protobuf::RpcController* controller,
                  const PhoneLoginReq* request, PhoneLoginRsp* response,
                  google::protobuf::Closure* done) {
    brpc::ClosureGuard rpc_guard(done);
    std::string request_id = request->request_id();
    response->set_request_id(request_id);

    auto err_rsp = [response](const std::string& errmsg) {
      response->set_success(false);
      response->set_errmsg(errmsg);
    };
    std::string phone = request->phone_number();
    std::string code_id = request->verify_code_id();
    std::string code = request->verify_code();

    if (!check_phone(phone)) {
      LOG_ERROR("{} 手机号格式不合法: {}", request_id, phone);
      err_rsp("手机号格式不合法");
      return;
    }

    auto user = _mysql_user->select_by_phone(phone);
    if (!user) {
      LOG_ERROR("{} 手机号不存在: {}", request_id, phone);
      err_rsp("手机号不存在");
      return;
    }

    auto verify_code = _redis_code->code(code_id);
    if (code != verify_code) {
      LOG_ERROR("{} 验证码错误: {}", request_id, code);
      err_rsp("验证码错误");
      return;
    }
    _redis_code->remove(code_id);

    bool ret = _redis_status->exists(user->user_id());
    if (ret) {
      LOG_ERROR("{} 用户已在其他地方登录: {}", request_id, phone);
      err_rsp("用户已在其他地方登录");
      return;
    }

    std::string login_session_id = uuid();
    ret = _redis_session->insert(login_session_id, user->user_id());
    if (!ret) {
      LOG_ERROR("{} redis新增用户会话失败", request_id);
      err_rsp("redis新增用户会话失败");
      return;
    }

    ret = _redis_status->insert(user->user_id());
    if (!ret) {
      LOG_ERROR("{} redis新增用户状态失败", request_id);
      err_rsp("redis新增用户状态失败");
      return;
    }

    response->set_success(true);
    response->set_login_session_id(login_session_id);
  }

  void GetUserInfo(google::protobuf::RpcController* controller,
                   const GetUserInfoReq* request, GetUserInfoRsp* response,
                   google::protobuf::Closure* done) {
    brpc::ClosureGuard rpc_guard(done);
    std::string request_id = request->request_id();
    response->set_request_id(request_id);

    auto err_rsp = [response](const std::string& errmsg) {
      response->set_success(false);
      response->set_errmsg(errmsg);
    };
    std::string user_id = request->user_id();

    auto user = _mysql_user->select_by_id(user_id);
    if (!user) {
      LOG_ERROR("{} 用户不存在: {}", request_id, user_id);
      err_rsp("用户不存在");
      return;
    }

    response->mutable_user_info()->set_user_id(user->user_id());
    response->mutable_user_info()->set_name(user->name());
    response->mutable_user_info()->set_phone(user->phone());
    response->mutable_user_info()->set_description(user->description());
    if (!user->avatar_id().empty()) {
      auto channel = _channels->get(_file_service_name);
      if (!channel) {
        LOG_ERROR("{} 未找到 {} 服务节点", request_id, _file_service_name);
        err_rsp("未找到服务节点");
        return;
      }

      huzch::FileService_Stub stub(channel.get());
      brpc::Controller ctrl;
      huzch::GetSingleFileReq req;
      req.set_request_id(request_id);
      req.set_file_id(user->avatar_id());
      huzch::GetSingleFileRsp rsp;

      stub.GetSingleFile(&ctrl, &req, &rsp, nullptr);
      if (ctrl.Failed() || !rsp.success()) {
        LOG_ERROR("{} {} 服务调用失败: {} {}", request_id, _file_service_name,
                  ctrl.ErrorText(), rsp.errmsg());
        err_rsp("服务调用失败");
        return;
      }

      response->mutable_user_info()->set_avatar(rsp.file_data().file_content());
    }

    response->set_success(true);
  }

  void GetMultiUserInfo(google::protobuf::RpcController* controller,
                        const GetMultiUserInfoReq* request,
                        GetMultiUserInfoRsp* response,
                        google::protobuf::Closure* done) {
    brpc::ClosureGuard rpc_guard(done);
    std::string request_id = request->request_id();
    response->set_request_id(request_id);

    auto err_rsp = [response](const std::string& errmsg) {
      response->set_success(false);
      response->set_errmsg(errmsg);
    };

    std::vector<std::string> users_id;
    users_id.reserve(request->users_id_size());
    for (size_t i = 0; i < request->users_id_size(); ++i) {
      users_id.push_back(request->users_id(i));
    }

    auto users = _mysql_user->select_by_multi_id(users_id);
    if (users.size() != users_id.size()) {
      LOG_ERROR("{} 用户查询结果与查找条件不一致 {} {}", request_id,
                users.size(), users_id.size());
      err_rsp("用户查询结果与查找条件不一致");
      return;
    }

    std::unordered_set<std::string> files_id;
    for (auto& user : users) {
      if (!user.avatar_id().empty()) {
        files_id.insert(user.avatar_id());
      }
    }
    std::unordered_map<std::string, std::string> files_data;
    bool ret = get_file(request_id, files_id, files_data);
    if (!ret) {
      LOG_ERROR("{} 批量下载文件失败", request_id);
      err_rsp("批量下载文件失败");
      return;
    }

    for (auto& user : users) {
      UserInfo user_info;
      user_info.set_user_id(user.user_id());
      user_info.set_name(user.name());
      user_info.set_phone(user.phone());
      user_info.set_description(user.description());
      if (!user.avatar_id().empty()) {
        user_info.set_avatar(files_data[user.avatar_id()]);
      }
      response->mutable_users_info()->insert({user.user_id(), user_info});
    }
    response->set_success(true);
  }

  void UserSearch(google::protobuf::RpcController* controller,
                  const UserSearchReq* request, UserSearchRsp* response,
                  google::protobuf::Closure* done) {
    brpc::ClosureGuard rpc_guard(done);
    std::string request_id = request->request_id();
    response->set_request_id(request_id);

    auto err_rsp = [response](const std::string& errmsg) {
      response->set_success(false);
      response->set_errmsg(errmsg);
    };
    std::string search_key = request->search_key();
    std::string user_id = request->user_id();

    std::vector<std::string> users_id;
    users_id.push_back(user_id);
    auto users = _es_user->search(search_key, users_id);

    std::unordered_set<std::string> files_id;
    for (auto& user : users) {
      if (!user.avatar_id().empty()) {
        files_id.insert(user.avatar_id());
      }
    }
    std::unordered_map<std::string, std::string> files_data;
    bool ret = get_file(request_id, files_id, files_data);
    if (!ret) {
      LOG_ERROR("{} 批量下载文件失败", request_id);
      err_rsp("批量下载文件失败");
      return;
    }

    for (auto& user : users) {
      auto user_info = response->add_users_info();
      user_info->set_user_id(user.user_id());
      user_info->set_name(user.name());
      user_info->set_phone(user.phone());
      user_info->set_description(user.description());
      if (!user.avatar_id().empty()) {
        user_info->set_avatar(files_data[user.avatar_id()]);
      }
    }
    response->set_success(true);
  }

  void SetUserAvatar(google::protobuf::RpcController* controller,
                     const SetUserAvatarReq* request,
                     SetUserAvatarRsp* response,
                     google::protobuf::Closure* done) {
    brpc::ClosureGuard rpc_guard(done);
    std::string request_id = request->request_id();
    response->set_request_id(request_id);

    auto err_rsp = [response](const std::string& errmsg) {
      response->set_success(false);
      response->set_errmsg(errmsg);
    };
    std::string user_id = request->user_id();
    std::string avatar = request->avatar();

    auto user = _mysql_user->select_by_id(user_id);
    if (!user) {
      LOG_ERROR("{} 用户不存在: {}", request_id, user_id);
      err_rsp("用户不存在");
      return;
    }

    auto channel = _channels->get(_file_service_name);
    if (!channel) {
      LOG_ERROR("{} 未找到 {} 服务节点", request_id, _file_service_name);
      err_rsp("未找到服务节点");
      return;
    }

    huzch::FileService_Stub stub(channel.get());
    brpc::Controller ctrl;
    huzch::PutSingleFileReq req;
    req.set_request_id(request_id);
    req.mutable_file_data()->set_file_size(avatar.size());
    req.mutable_file_data()->set_file_content(avatar);
    huzch::PutSingleFileRsp rsp;

    stub.PutSingleFile(&ctrl, &req, &rsp, nullptr);
    if (ctrl.Failed() || !rsp.success()) {
      LOG_ERROR("{} {} 服务调用失败: {} {}", request_id, _file_service_name,
                ctrl.ErrorText(), rsp.errmsg());
      err_rsp("服务调用失败");
      return;
    }

    user->avatar_id(rsp.file_info().file_id());
    bool ret = _mysql_user->update(user);
    if (!ret) {
      LOG_ERROR("{} mysql更新用户头像id失败: {}", request_id,
                user->avatar_id());
      err_rsp("mysql更新用户头像id失败");
      return;
    }

    ret = _es_user->insert(user_id, user->avatar_id(), user->name(),
                           user->phone(), user->description());
    if (!ret) {
      LOG_ERROR("{} es更新用户头像id失败: {}", request_id, user->avatar_id());
      err_rsp("es更新用户头像id失败");
      return;
    }

    response->set_success(true);
  }

  void SetUserName(google::protobuf::RpcController* controller,
                   const SetUserNameReq* request, SetUserNameRsp* response,
                   google::protobuf::Closure* done) {
    brpc::ClosureGuard rpc_guard(done);
    std::string request_id = request->request_id();
    response->set_request_id(request_id);

    auto err_rsp = [response](const std::string& errmsg) {
      response->set_success(false);
      response->set_errmsg(errmsg);
    };
    std::string user_id = request->user_id();
    std::string name = request->user_name();

    if (!check_name(name)) {
      LOG_ERROR("{} 用户名长度不合法: {}", request_id, name);
      err_rsp("用户名长度不合法");
      return;
    }

    auto user = _mysql_user->select_by_id(user_id);
    if (!user) {
      LOG_ERROR("{} 用户不存在: {}", request_id, user_id);
      err_rsp("用户不存在");
      return;
    }

    user->name(name);
    bool ret = _mysql_user->update(user);
    if (!ret) {
      LOG_ERROR("{} mysql更新用户名失败: {}", request_id, user->name());
      err_rsp("mysql更新用户名失败");
      return;
    }

    ret = _es_user->insert(user_id, user->avatar_id(), user->name(),
                           user->phone(), user->description());
    if (!ret) {
      LOG_ERROR("{} es更新用户名失败: {}", request_id, user->name());
      err_rsp("es更新用户名失败");
      return;
    }

    response->set_success(true);
  }

  void SetUserDescription(google::protobuf::RpcController* controller,
                          const SetUserDescriptionReq* request,
                          SetUserDescriptionRsp* response,
                          google::protobuf::Closure* done) {
    brpc::ClosureGuard rpc_guard(done);
    std::string request_id = request->request_id();
    response->set_request_id(request_id);

    auto err_rsp = [response](const std::string& errmsg) {
      response->set_success(false);
      response->set_errmsg(errmsg);
    };
    std::string user_id = request->user_id();
    std::string description = request->description();

    auto user = _mysql_user->select_by_id(user_id);
    if (!user) {
      LOG_ERROR("{} 用户不存在: {}", request_id, user_id);
      err_rsp("用户不存在");
      return;
    }

    user->description(description);
    bool ret = _mysql_user->update(user);
    if (!ret) {
      LOG_ERROR("{} mysql更新用户签名失败: {}", request_id,
                user->description());
      err_rsp("mysql更新用户签名失败");
      return;
    }

    ret = _es_user->insert(user_id, user->avatar_id(), user->name(),
                           user->phone(), user->description());
    if (!ret) {
      LOG_ERROR("{} es更新用户签名失败: {}", request_id, user->description());
      err_rsp("es更新用户签名失败");
      return;
    }

    response->set_success(true);
  }

  void SetUserPhoneNumber(google::protobuf::RpcController* controller,
                          const SetUserPhoneNumberReq* request,
                          SetUserPhoneNumberRsp* response,
                          google::protobuf::Closure* done) {
    brpc::ClosureGuard rpc_guard(done);
    std::string request_id = request->request_id();
    response->set_request_id(request_id);

    auto err_rsp = [response](const std::string& errmsg) {
      response->set_success(false);
      response->set_errmsg(errmsg);
    };
    std::string user_id = request->user_id();
    std::string phone = request->phone_number();
    std::string code_id = request->phone_verify_code_id();
    std::string code = request->phone_verify_code();

    if (!check_phone(phone)) {
      LOG_ERROR("{} 手机号格式不合法: {}", request_id, phone);
      err_rsp("手机号格式不合法");
      return;
    }

    auto verify_code = _redis_code->code(code_id);
    if (code != verify_code) {
      LOG_ERROR("{} 验证码错误: {}", request_id, code);
      err_rsp("验证码错误");
      return;
    }
    _redis_code->remove(code_id);

    auto user = _mysql_user->select_by_id(user_id);
    if (!user) {
      LOG_ERROR("{} 用户不存在: {}", request_id, user_id);
      err_rsp("用户不存在");
      return;
    }

    user->phone(phone);
    bool ret = _mysql_user->update(user);
    if (!ret) {
      LOG_ERROR("{} mysql更新用户手机号失败: {}", request_id, user->phone());
      err_rsp("mysql更新用户手机号失败");
      return;
    }

    ret = _es_user->insert(user_id, user->avatar_id(), user->name(),
                           user->phone(), user->description());
    if (!ret) {
      LOG_ERROR("{} es更新用户手机号失败: {}", request_id, user->phone());
      err_rsp("es更新用户手机号失败");
      return;
    }

    response->set_success(true);
  }

 private:
  bool get_file(const std::string& request_id,
                const std::unordered_set<std::string> files_id,
                std::unordered_map<std::string, std::string>& files_data) {
    auto channel = _channels->get(_file_service_name);
    if (!channel) {
      LOG_ERROR("{} 未找到 {} 服务节点", request_id, _file_service_name);
      return false;
    }

    huzch::FileService_Stub stub(channel.get());
    brpc::Controller ctrl;
    huzch::GetMultiFileReq req;
    req.set_request_id(request_id);
    for (const auto& file_id : files_id) {
      req.add_files_id(file_id);
    }
    huzch::GetMultiFileRsp rsp;

    stub.GetMultiFile(&ctrl, &req, &rsp, nullptr);
    if (ctrl.Failed() || !rsp.success()) {
      LOG_ERROR("{} {} 服务调用失败: {} {}", request_id, _file_service_name,
                ctrl.ErrorText(), rsp.errmsg());
      return false;
    }

    for (const auto& [key, val] : rsp.files_data()) {
      files_data[key] = val.file_content();
    }
    return true;
  }

  bool check_name(const std::string& name) { return name.size() <= 21; }

  bool check_password(const std::string& password) {
    // 检查密码长度：6-20位
    if (password.size() < 6 || password.size() > 20) {
      LOG_ERROR("密码长度不合法: {}", password.size());
      return false;
    }

    // 检查是否包含至少一个字母和一个数字
    bool has_letter = false;
    bool has_digit = false;

    for (char c : password) {
      // 检查字符是否合法（字母、数字、常见特殊字符）
      if (!std::isalnum(c) && c != '_' && c != '@' && c != '#' && c != '$' &&
          c != '%' && c != '^' && c != '&' && c != '*' && c != '!' &&
          c != '?') {
        LOG_ERROR("密码字符不合法: {}", password);
        return false;
      }

      if (std::isalpha(c)) {
        has_letter = true;
      }
      if (std::isdigit(c)) {
        has_digit = true;
      }
    }

    // 密码必须包含至少一个字母和一个数字
    if (!has_letter || !has_digit) {
      LOG_ERROR("密码必须包含至少一个字母和一个数字: {}", password);
      return false;
    }
    return true;
  }

  bool check_phone(const std::string& phone) {
    if (phone.size() != 11) {
      return false;
    }

    if (phone[0] != '1') {
      return false;
    }

    if (phone[1] < '3' || phone[1] > '9') {
      return false;
    }

    for (size_t i = 2; i < 11; ++i) {
      if (!std::isdigit(phone[i])) {
        return false;
      }
    }
    return true;
  }

 private:
  ESUser::Ptr _es_user;
  UserTable::Ptr _mysql_user;
  Session::Ptr _redis_session;
  Status::Ptr _redis_status;
  Code::Ptr _redis_code;

  SMSClient::Ptr _sms_client;
  std::string _file_service_name;
  ChannelManager::Ptr _channels;
};

class UserServer {
 public:
  using Ptr = std::shared_ptr<UserServer>;

 public:
  UserServer(const std::shared_ptr<brpc::Server>& server) : _server(server) {}

  // 启动服务器
  void start() { _server->RunUntilAskedToQuit(); }

 private:
  std::shared_ptr<brpc::Server> _server;
};

class UserServerBuilder {
 public:
  void init_registry_client(const std::string& registry_host,
                            const std::string& service_name,
                            const std::string& service_host) {
    _registry_client = std::make_shared<ServiceRegistry>(registry_host);
    _registry_client->register_service(service_name, service_host);
  }

  void init_discovery_client(const std::string& registry_host,
                             const std::string& base_dir,
                             const std::string& service_name) {
    _file_service_name = base_dir + service_name;
    _channels = std::make_shared<ChannelManager>();
    _channels->declare(base_dir + service_name);
    auto put_cb = std::bind(&ChannelManager::on_service_online, _channels.get(),
                            std::placeholders::_1, std::placeholders::_2);
    auto del_cb =
        std::bind(&ChannelManager::on_service_offline, _channels.get(),
                  std::placeholders::_1, std::placeholders::_2);

    _discovery_client = std::make_shared<ServiceDiscovery>(
        registry_host, base_dir, put_cb, del_cb);
  }

  void init_sms_client(const std::string& key_id) {
    _sms_client = std::make_shared<SMSClient>(key_id);
  }

  void init_es_client(const std::vector<std::string>& hosts) {
    _es_client = ESClientFactory::create(hosts);
  }

  void init_mysql_client(const std::string& user, const std::string& passwd,
                         const std::string& db, const std::string& host,
                         size_t port, const std::string& charset,
                         size_t max_connections) {
    _mysql_client = MysqlClientFactory::create(user, passwd, db, host, port,
                                               charset, max_connections);
  }

  void init_redis_client(const std::string& host, int port, int db,
                         bool keep_alive) {
    _redis_client = RedisClientFactory::create(host, port, db, keep_alive);
  }

  void init_rpc_server(int port, int timeout, int num_threads) {
    if (!_sms_client) {
      LOG_ERROR("未初始化短信发送模块");
      abort();
    }

    if (!_es_client) {
      LOG_ERROR("未初始化es搜索引擎模块");
      abort();
    }

    if (!_mysql_client) {
      LOG_ERROR("未初始化mysql数据库模块");
      abort();
    }

    if (!_redis_client) {
      LOG_ERROR("未初始化redis数据库模块");
      abort();
    }

    _server = std::make_shared<brpc::Server>();
    auto user_service =
        new UserServiceImpl(_es_client, _mysql_client, _redis_client,
                            _sms_client, _file_service_name, _channels);
    int ret = _server->AddService(user_service,
                                  brpc::ServiceOwnership::SERVER_OWNS_SERVICE);
    if (ret == -1) {
      LOG_ERROR("服务添加失败");
      abort();
    }

    brpc::ServerOptions options;
    options.idle_timeout_sec = timeout;
    options.num_threads = num_threads;
    ret = _server->Start(port, &options);
    if (ret == -1) {
      LOG_ERROR("服务启动失败");
      abort();
    }
  }

  UserServer::Ptr build() {
    if (!_registry_client) {
      LOG_ERROR("未初始化服务注册模块");
      abort();
    }

    if (!_discovery_client) {
      LOG_ERROR("未初始化服务发现模块");
      abort();
    }

    if (!_channels) {
      LOG_ERROR("未初始化服务信道管理模块");
      abort();
    }

    if (!_server) {
      LOG_ERROR("未初始化rpc服务器模块");
      abort();
    }

    return std::make_shared<UserServer>(_server);
  }

 private:
  ServiceRegistry::Ptr _registry_client;
  ServiceDiscovery::Ptr _discovery_client;
  SMSClient::Ptr _sms_client;
  std::shared_ptr<elasticlient::Client> _es_client;
  std::shared_ptr<odb::core::database> _mysql_client;
  std::shared_ptr<sw::redis::Redis> _redis_client;
  std::shared_ptr<brpc::Server> _server;

  std::string _file_service_name;
  ChannelManager::Ptr _channels;
};

}  // namespace huzch