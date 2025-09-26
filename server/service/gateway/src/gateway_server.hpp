#pragma once

#include "base.pb.h"
#include "channel.hpp"
#include "connection.hpp"
#include "data_redis.hpp"
#include "etcd.hpp"
#include "file.pb.h"
#include "forward.pb.h"
#include "friend.pb.h"
#include "gateway.pb.h"
#include "httplib.h"
#include "message.pb.h"
#include "notify.pb.h"
#include "speech.pb.h"
#include "user.pb.h"

namespace huzch {

#define SPEECH_RECOGNIZE "/service/speech/speech_recognize"
#define GET_SINGLE_FILE "/service/file/get_single_file"
#define GET_MULTI_FILE "/service/file/get_multi_file"
#define PUT_SINGLE_FILE "/service/file/put_single_file"
#define PUT_MULTI_FILE "/service/file/put_multi_file"
#define USER_REGISTER "/service/user/user_register"
#define USER_LOGIN "/service/user/user_login"
#define GET_PHONE_VERIFY_CODE "/service/user/get_phone_verify_code"
#define PHONE_REGISTER "/service/user/phone_register"
#define PHONE_LOGIN "/service/user/phone_login"
#define GET_USER_INFO "/service/user/get_user_info"
#define USER_SEARCH "/service/user/user_search"
#define SET_USER_AVATAR "/service/user/set_user_avatar"
#define SET_USER_NAME "/service/user/set_user_name"
#define SET_USER_PHONE "/service/user/set_user_phone"
#define SET_USER_DESCRIPTION "/service/user/set_user_description"
#define NEW_MESSAGE "/service/forward/new_message"
#define GET_HISTORY_MESSAGE "/service/message/get_history_message"
#define GET_RECENT_MESSAGE "/service/message/get_recent_message"
#define MESSAGE_SEARCH "/service/message/message_search"
#define GET_FRIEND "/service/friend/get_friend"
#define FRIEND_REMOVE "/service/friend/friend_remove"
#define FRIEND_ADD_SEND "/service/friend/friend_add_send"
#define FRIEND_ADD_PROCESS "/service/friend/friend_add_process"
#define GET_REQUESTER "/service/friend/get_requester"
#define GET_CHAT_SESSION "/service/friend/get_chat_session"
#define CHAT_SESSION_CREATE "/service/friend/chat_session_create"
#define GET_CHAT_SESSION_MEMBER "/service/friend/get_chat_session_member"

class GatewayServer {
 public:
  using Ptr = std::shared_ptr<GatewayServer>;
  using CallBack =
      std::function<void(const httplib::Request&, httplib::Response&)>;

 public:
  GatewayServer(int http_port, int websocket_port,
                const std::shared_ptr<sw::redis::Redis>& redis_client,
                const std::string& speech_service_name,
                const std::string& file_service_name,
                const std::string& user_service_name,
                const std::string& forward_service_name,
                const std::string& message_service_name,
                const std::string& friend_service_name,
                const ChannelManager::Ptr& channels)
      : _redis_session(std::make_shared<Session>(redis_client)),
        _redis_status(std::make_shared<Status>(redis_client)),
        _speech_service_name(speech_service_name),
        _file_service_name(file_service_name),
        _user_service_name(user_service_name),
        _forward_service_name(forward_service_name),
        _message_service_name(message_service_name),
        _friend_service_name(friend_service_name),
        _channels(channels),
        _connections(std::make_shared<ConnectionManager>()) {
    // 取消打印所有日志
    _websocket_server.set_access_channels(websocketpp::log::alevel::none);
    // 初始化asio框架中的io_service调度器
    _websocket_server.init_asio();
    // 设置connection开启回调函数
    _websocket_server.set_open_handler(
        std::bind(&GatewayServer::on_open, this, std::placeholders::_1));
    // 设置connection关闭回调函数
    _websocket_server.set_close_handler(
        std::bind(&GatewayServer::on_close, this, std::placeholders::_1));
    // 设置message回调函数
    _websocket_server.set_message_handler(std::bind(&GatewayServer::on_message,
                                                    this, std::placeholders::_1,
                                                    std::placeholders::_2));
    // 启用地址重用
    _websocket_server.set_reuse_addr(true);
    // 设置endpoint监听端口
    _websocket_server.listen(websocket_port);
    // 启动服务器connection接受循环
    _websocket_server.start_accept();

    _http_server.Post(
        SPEECH_RECOGNIZE,
        (CallBack)std::bind(&GatewayServer::SpeechRecognize, this,
                            std::placeholders::_1, std::placeholders::_2));
    _http_server.Post(
        GET_SINGLE_FILE,
        (CallBack)std::bind(&GatewayServer::GetSingleFile, this,
                            std::placeholders::_1, std::placeholders::_2));
    _http_server.Post(
        GET_MULTI_FILE,
        (CallBack)std::bind(&GatewayServer::GetMultiFile, this,
                            std::placeholders::_1, std::placeholders::_2));
    _http_server.Post(
        PUT_SINGLE_FILE,
        (CallBack)std::bind(&GatewayServer::PutSingleFile, this,
                            std::placeholders::_1, std::placeholders::_2));
    _http_server.Post(
        PUT_MULTI_FILE,
        (CallBack)std::bind(&GatewayServer::PutMultiFile, this,
                            std::placeholders::_1, std::placeholders::_2));
    _http_server.Post(
        USER_REGISTER,
        (CallBack)std::bind(&GatewayServer::UserRegister, this,
                            std::placeholders::_1, std::placeholders::_2));
    _http_server.Post(
        USER_LOGIN,
        (CallBack)std::bind(&GatewayServer::UserLogin, this,
                            std::placeholders::_1, std::placeholders::_2));
    _http_server.Post(
        GET_PHONE_VERIFY_CODE,
        (CallBack)std::bind(&GatewayServer::GetPhoneVerifyCode, this,
                            std::placeholders::_1, std::placeholders::_2));
    _http_server.Post(
        PHONE_REGISTER,
        (CallBack)std::bind(&GatewayServer::PhoneRegister, this,
                            std::placeholders::_1, std::placeholders::_2));
    _http_server.Post(
        PHONE_LOGIN,
        (CallBack)std::bind(&GatewayServer::PhoneLogin, this,
                            std::placeholders::_1, std::placeholders::_2));
    _http_server.Post(
        GET_USER_INFO,
        (CallBack)std::bind(&GatewayServer::GetUserInfo, this,
                            std::placeholders::_1, std::placeholders::_2));
    _http_server.Post(
        USER_SEARCH,
        (CallBack)std::bind(&GatewayServer::UserSearch, this,
                            std::placeholders::_1, std::placeholders::_2));
    _http_server.Post(
        SET_USER_AVATAR,
        (CallBack)std::bind(&GatewayServer::SetUserAvatar, this,
                            std::placeholders::_1, std::placeholders::_2));
    _http_server.Post(
        SET_USER_NAME,
        (CallBack)std::bind(&GatewayServer::SetUserName, this,
                            std::placeholders::_1, std::placeholders::_2));
    _http_server.Post(
        SET_USER_PHONE,
        (CallBack)std::bind(&GatewayServer::SetUserPhoneNumber, this,
                            std::placeholders::_1, std::placeholders::_2));
    _http_server.Post(
        SET_USER_DESCRIPTION,
        (CallBack)std::bind(&GatewayServer::SetUserDescription, this,
                            std::placeholders::_1, std::placeholders::_2));
    _http_server.Post(
        NEW_MESSAGE,
        (CallBack)std::bind(&GatewayServer::NewMessage, this,
                            std::placeholders::_1, std::placeholders::_2));
    _http_server.Post(
        GET_HISTORY_MESSAGE,
        (CallBack)std::bind(&GatewayServer::GetHistoryMessage, this,
                            std::placeholders::_1, std::placeholders::_2));
    _http_server.Post(
        GET_RECENT_MESSAGE,
        (CallBack)std::bind(&GatewayServer::GetRecentMessage, this,
                            std::placeholders::_1, std::placeholders::_2));
    _http_server.Post(
        MESSAGE_SEARCH,
        (CallBack)std::bind(&GatewayServer::MessageSearch, this,
                            std::placeholders::_1, std::placeholders::_2));
    _http_server.Post(
        GET_FRIEND,
        (CallBack)std::bind(&GatewayServer::GetFriend, this,
                            std::placeholders::_1, std::placeholders::_2));
    _http_server.Post(
        FRIEND_REMOVE,
        (CallBack)std::bind(&GatewayServer::FriendRemove, this,
                            std::placeholders::_1, std::placeholders::_2));
    _http_server.Post(
        FRIEND_ADD_SEND,
        (CallBack)std::bind(&GatewayServer::FriendAddSend, this,
                            std::placeholders::_1, std::placeholders::_2));
    _http_server.Post(
        FRIEND_ADD_PROCESS,
        (CallBack)std::bind(&GatewayServer::FriendAddProcess, this,
                            std::placeholders::_1, std::placeholders::_2));
    _http_server.Post(
        GET_REQUESTER,
        (CallBack)std::bind(&GatewayServer::GetRequester, this,
                            std::placeholders::_1, std::placeholders::_2));
    _http_server.Post(
        GET_CHAT_SESSION,
        (CallBack)std::bind(&GatewayServer::GetChatSession, this,
                            std::placeholders::_1, std::placeholders::_2));
    _http_server.Post(
        CHAT_SESSION_CREATE,
        (CallBack)std::bind(&GatewayServer::ChatSessionCreate, this,
                            std::placeholders::_1, std::placeholders::_2));
    _http_server.Post(
        GET_CHAT_SESSION_MEMBER,
        (CallBack)std::bind(&GatewayServer::GetChatSessionMember, this,
                            std::placeholders::_1, std::placeholders::_2));

    _http_thread = std::thread(
        [this, http_port]() { _http_server.listen("0.0.0.0", http_port); });
    _http_thread.detach();
  }

  void start() { _websocket_server.run(); }

 private:
  void on_open(websocketpp::connection_hdl hdl) {
    auto connection = _websocket_server.get_con_from_hdl(hdl);
    LOG_INFO("websocket长连接建立成功 {}", (size_t)connection.get());
  }

  void on_close(websocketpp::connection_hdl hdl) {
    auto connection = _websocket_server.get_con_from_hdl(hdl);

    std::string user_id, session_id;
    bool ret = _connections->client(connection, user_id, session_id);
    if (!ret) {
      LOG_WARN("websocket长连接不存在 {}", (size_t)connection.get());
      return;
    }

    ret = _redis_session->remove(session_id);
    if (!ret) {
      LOG_ERROR("redis移除用户会话失败");
      return;
    }

    ret = _redis_status->remove(user_id);
    if (!ret) {
      LOG_ERROR("redis移除用户状态失败");
      return;
    }

    _connections->remove(connection);
    LOG_INFO("websocket长连接断开成功 {}", (size_t)connection.get());
  }

  void on_message(
      websocketpp::connection_hdl hdl,
      websocketpp::server<websocketpp::config::asio>::message_ptr message) {
    auto connection = _websocket_server.get_con_from_hdl(hdl);

    ClientAuthenticationReq req;
    bool ret = req.ParseFromString(message->get_payload());
    if (!ret) {
      LOG_ERROR("请求正文反序列化失败");
      _websocket_server.close(hdl, websocketpp::close::status::invalid_payload,
                              "请求正文反序列化失败");
      return;
    }

    std::string login_session_id = req.login_session_id();
    auto user_id = _redis_session->user_id(login_session_id);
    if (!user_id) {
      LOG_ERROR("登录会话不存在");
      _websocket_server.close(hdl, websocketpp::close::status::unsupported_data,
                              "登录会话不存在");
      return;
    }

    _connections->insert(connection, *user_id, login_session_id);
    LOG_INFO("websocket长连接建立成功 {}", (size_t)connection.get());
  }

  void SpeechRecognize(const httplib::Request& request,
                       httplib::Response& response) {
    SpeechRecognizeReq req;
    SpeechRecognizeRsp rsp;
    auto err_rsp = [&req, &rsp, &response](const std::string& errmsg) {
      rsp.set_success(false);
      rsp.set_errmsg(errmsg);
      response.set_content(rsp.SerializeAsString(), "application/protobuf");
    };

    bool ret = req.ParseFromString(request.body);
    if (!ret) {
      LOG_ERROR("请求正文反序列化失败");
      err_rsp("请求正文反序列化失败");
      return;
    }

    std::string login_session_id = req.login_session_id();
    auto user_id = _redis_session->user_id(login_session_id);
    if (!user_id) {
      LOG_ERROR("登录会话不存在");
      err_rsp("登录会话不存在");
      return;
    }
    req.set_user_id(*user_id);

    auto channel = _channels->get(_speech_service_name);
    if (!channel) {
      LOG_ERROR("{} 服务节点不存在", _speech_service_name);
      err_rsp("服务节点不存在");
      return;
    }

    huzch::SpeechService_Stub stub(channel.get());
    brpc::Controller ctrl;
    stub.SpeechRecognize(&ctrl, &req, &rsp, nullptr);
    if (ctrl.Failed() || !rsp.success()) {
      LOG_ERROR("{} {} 服务调用失败: {} {}", req.request_id(),
                _speech_service_name, ctrl.ErrorText(), rsp.errmsg());
      err_rsp("服务调用失败");
      return;
    }

    response.set_content(rsp.SerializeAsString(), "application/protobuf");
  }

  void GetSingleFile(const httplib::Request& request,
                     httplib::Response& response) {
    GetSingleFileReq req;
    GetSingleFileRsp rsp;
    auto err_rsp = [&req, &rsp, &response](const std::string& errmsg) {
      rsp.set_success(false);
      rsp.set_errmsg(errmsg);
      response.set_content(rsp.SerializeAsString(), "application/protobuf");
    };

    bool ret = req.ParseFromString(request.body);
    if (!ret) {
      LOG_ERROR("请求正文反序列化失败");
      err_rsp("请求正文反序列化失败");
      return;
    }

    std::string login_session_id = req.login_session_id();
    auto user_id = _redis_session->user_id(login_session_id);
    if (!user_id) {
      LOG_ERROR("登录会话不存在");
      err_rsp("登录会话不存在");
      return;
    }
    req.set_user_id(*user_id);

    auto channel = _channels->get(_file_service_name);
    if (!channel) {
      LOG_ERROR("{} 服务节点不存在", _file_service_name);
      err_rsp("服务节点不存在");
      return;
    }

    huzch::FileService_Stub stub(channel.get());
    brpc::Controller ctrl;
    stub.GetSingleFile(&ctrl, &req, &rsp, nullptr);
    if (ctrl.Failed() || !rsp.success()) {
      LOG_ERROR("{} {} 服务调用失败: {} {}", req.request_id(),
                _file_service_name, ctrl.ErrorText(), rsp.errmsg());
      err_rsp("服务调用失败");
      return;
    }

    response.set_content(rsp.SerializeAsString(), "application/protobuf");
  }

  void GetMultiFile(const httplib::Request& request,
                    httplib::Response& response) {
    GetMultiFileReq req;
    GetMultiFileRsp rsp;
    auto err_rsp = [&req, &rsp, &response](const std::string& errmsg) {
      rsp.set_success(false);
      rsp.set_errmsg(errmsg);
      response.set_content(rsp.SerializeAsString(), "application/protobuf");
    };

    bool ret = req.ParseFromString(request.body);
    if (!ret) {
      LOG_ERROR("请求正文反序列化失败");
      err_rsp("请求正文反序列化失败");
      return;
    }

    std::string login_session_id = req.login_session_id();
    auto user_id = _redis_session->user_id(login_session_id);
    if (!user_id) {
      LOG_ERROR("登录会话不存在");
      err_rsp("登录会话不存在");
      return;
    }
    req.set_user_id(*user_id);

    auto channel = _channels->get(_file_service_name);
    if (!channel) {
      LOG_ERROR("{} 服务节点不存在", _file_service_name);
      err_rsp("服务节点不存在");
      return;
    }

    huzch::FileService_Stub stub(channel.get());
    brpc::Controller ctrl;
    stub.GetMultiFile(&ctrl, &req, &rsp, nullptr);
    if (ctrl.Failed() || !rsp.success()) {
      LOG_ERROR("{} {} 服务调用失败: {} {}", req.request_id(),
                _file_service_name, ctrl.ErrorText(), rsp.errmsg());
      err_rsp("服务调用失败");
      return;
    }

    response.set_content(rsp.SerializeAsString(), "application/protobuf");
  }

  void PutSingleFile(const httplib::Request& request,
                     httplib::Response& response) {
    PutSingleFileReq req;
    PutSingleFileRsp rsp;
    auto err_rsp = [&req, &rsp, &response](const std::string& errmsg) {
      rsp.set_success(false);
      rsp.set_errmsg(errmsg);
      response.set_content(rsp.SerializeAsString(), "application/protobuf");
    };

    bool ret = req.ParseFromString(request.body);
    if (!ret) {
      LOG_ERROR("请求正文反序列化失败");
      err_rsp("请求正文反序列化失败");
      return;
    }

    std::string login_session_id = req.login_session_id();
    auto user_id = _redis_session->user_id(login_session_id);
    if (!user_id) {
      LOG_ERROR("登录会话不存在");
      err_rsp("登录会话不存在");
      return;
    }
    req.set_user_id(*user_id);

    auto channel = _channels->get(_file_service_name);
    if (!channel) {
      LOG_ERROR("{} 服务节点不存在", _file_service_name);
      err_rsp("服务节点不存在");
      return;
    }

    huzch::FileService_Stub stub(channel.get());
    brpc::Controller ctrl;
    stub.PutSingleFile(&ctrl, &req, &rsp, nullptr);
    if (ctrl.Failed() || !rsp.success()) {
      LOG_ERROR("{} {} 服务调用失败: {} {}", req.request_id(),
                _file_service_name, ctrl.ErrorText(), rsp.errmsg());
      err_rsp("服务调用失败");
      return;
    }

    response.set_content(rsp.SerializeAsString(), "application/protobuf");
  }

  void PutMultiFile(const httplib::Request& request,
                    httplib::Response& response) {
    PutMultiFileReq req;
    PutMultiFileRsp rsp;
    auto err_rsp = [&req, &rsp, &response](const std::string& errmsg) {
      rsp.set_success(false);
      rsp.set_errmsg(errmsg);
      response.set_content(rsp.SerializeAsString(), "application/protobuf");
    };

    bool ret = req.ParseFromString(request.body);
    if (!ret) {
      LOG_ERROR("请求正文反序列化失败");
      err_rsp("请求正文反序列化失败");
      return;
    }

    std::string login_session_id = req.login_session_id();
    auto user_id = _redis_session->user_id(login_session_id);
    if (!user_id) {
      LOG_ERROR("登录会话不存在");
      err_rsp("登录会话不存在");
      return;
    }
    req.set_user_id(*user_id);

    auto channel = _channels->get(_file_service_name);
    if (!channel) {
      LOG_ERROR("{} 服务节点不存在", _file_service_name);
      err_rsp("服务节点不存在");
      return;
    }

    huzch::FileService_Stub stub(channel.get());
    brpc::Controller ctrl;
    stub.PutMultiFile(&ctrl, &req, &rsp, nullptr);
    if (ctrl.Failed() || !rsp.success()) {
      LOG_ERROR("{} {} 服务调用失败: {} {}", req.request_id(),
                _file_service_name, ctrl.ErrorText(), rsp.errmsg());
      err_rsp("服务调用失败");
      return;
    }

    response.set_content(rsp.SerializeAsString(), "application/protobuf");
  }

  void UserRegister(const httplib::Request& request,
                    httplib::Response& response) {
    UserRegisterReq req;
    UserRegisterRsp rsp;
    auto err_rsp = [&req, &rsp, &response](const std::string& errmsg) {
      rsp.set_success(false);
      rsp.set_errmsg(errmsg);
      response.set_content(rsp.SerializeAsString(), "application/protobuf");
    };

    bool ret = req.ParseFromString(request.body);
    if (!ret) {
      LOG_ERROR("请求正文反序列化失败");
      err_rsp("请求正文反序列化失败");
      return;
    }

    auto channel = _channels->get(_user_service_name);
    if (!channel) {
      LOG_ERROR("{} 服务节点不存在", _user_service_name);
      err_rsp("服务节点不存在");
      return;
    }

    huzch::UserService_Stub stub(channel.get());
    brpc::Controller ctrl;
    stub.UserRegister(&ctrl, &req, &rsp, nullptr);
    if (ctrl.Failed() || !rsp.success()) {
      LOG_ERROR("{} {} 服务调用失败: {} {}", req.request_id(),
                _user_service_name, ctrl.ErrorText(), rsp.errmsg());
      err_rsp("服务调用失败");
      return;
    }

    response.set_content(rsp.SerializeAsString(), "application/protobuf");
  }

  void UserLogin(const httplib::Request& request, httplib::Response& response) {
    UserLoginReq req;
    UserLoginRsp rsp;
    auto err_rsp = [&req, &rsp, &response](const std::string& errmsg) {
      rsp.set_success(false);
      rsp.set_errmsg(errmsg);
      response.set_content(rsp.SerializeAsString(), "application/protobuf");
    };

    bool ret = req.ParseFromString(request.body);
    if (!ret) {
      LOG_ERROR("请求正文反序列化失败");
      err_rsp("请求正文反序列化失败");
      return;
    }

    auto channel = _channels->get(_user_service_name);
    if (!channel) {
      LOG_ERROR("{} 服务节点不存在", _user_service_name);
      err_rsp("服务节点不存在");
      return;
    }

    huzch::UserService_Stub stub(channel.get());
    brpc::Controller ctrl;
    stub.UserLogin(&ctrl, &req, &rsp, nullptr);
    if (ctrl.Failed() || !rsp.success()) {
      LOG_ERROR("{} {} 服务调用失败: {} {}", req.request_id(),
                _user_service_name, ctrl.ErrorText(), rsp.errmsg());
      err_rsp("服务调用失败");
      return;
    }

    response.set_content(rsp.SerializeAsString(), "application/protobuf");
  }

  void GetPhoneVerifyCode(const httplib::Request& request,
                          httplib::Response& response) {
    GetPhoneVerifyCodeReq req;
    GetPhoneVerifyCodeRsp rsp;
    auto err_rsp = [&req, &rsp, &response](const std::string& errmsg) {
      rsp.set_success(false);
      rsp.set_errmsg(errmsg);
      response.set_content(rsp.SerializeAsString(), "application/protobuf");
    };

    bool ret = req.ParseFromString(request.body);
    if (!ret) {
      LOG_ERROR("请求正文反序列化失败");
      err_rsp("请求正文反序列化失败");
      return;
    }

    auto channel = _channels->get(_user_service_name);
    if (!channel) {
      LOG_ERROR("{} 服务节点不存在", _user_service_name);
      err_rsp("服务节点不存在");
      return;
    }

    huzch::UserService_Stub stub(channel.get());
    brpc::Controller ctrl;
    stub.GetPhoneVerifyCode(&ctrl, &req, &rsp, nullptr);
    if (ctrl.Failed() || !rsp.success()) {
      LOG_ERROR("{} {} 服务调用失败: {} {}", req.request_id(),
                _user_service_name, ctrl.ErrorText(), rsp.errmsg());
      err_rsp("服务调用失败");
      return;
    }

    response.set_content(rsp.SerializeAsString(), "application/protobuf");
  }

  void PhoneRegister(const httplib::Request& request,
                     httplib::Response& response) {
    PhoneRegisterReq req;
    PhoneRegisterRsp rsp;
    auto err_rsp = [&req, &rsp, &response](const std::string& errmsg) {
      rsp.set_success(false);
      rsp.set_errmsg(errmsg);
      response.set_content(rsp.SerializeAsString(), "application/protobuf");
    };

    bool ret = req.ParseFromString(request.body);
    if (!ret) {
      LOG_ERROR("请求正文反序列化失败");
      err_rsp("请求正文反序列化失败");
      return;
    }

    auto channel = _channels->get(_user_service_name);
    if (!channel) {
      LOG_ERROR("{} 服务节点不存在", _user_service_name);
      err_rsp("服务节点不存在");
      return;
    }

    huzch::UserService_Stub stub(channel.get());
    brpc::Controller ctrl;
    stub.PhoneRegister(&ctrl, &req, &rsp, nullptr);
    if (ctrl.Failed() || !rsp.success()) {
      LOG_ERROR("{} {} 服务调用失败: {} {}", req.request_id(),
                _user_service_name, ctrl.ErrorText(), rsp.errmsg());
      err_rsp("服务调用失败");
      return;
    }

    response.set_content(rsp.SerializeAsString(), "application/protobuf");
  }

  void PhoneLogin(const httplib::Request& request,
                  httplib::Response& response) {
    PhoneLoginReq req;
    PhoneLoginRsp rsp;
    auto err_rsp = [&req, &rsp, &response](const std::string& errmsg) {
      rsp.set_success(false);
      rsp.set_errmsg(errmsg);
      response.set_content(rsp.SerializeAsString(), "application/protobuf");
    };

    bool ret = req.ParseFromString(request.body);
    if (!ret) {
      LOG_ERROR("请求正文反序列化失败");
      err_rsp("请求正文反序列化失败");
      return;
    }

    auto channel = _channels->get(_user_service_name);
    if (!channel) {
      LOG_ERROR("{} 服务节点不存在", _user_service_name);
      err_rsp("服务节点不存在");
      return;
    }

    huzch::UserService_Stub stub(channel.get());
    brpc::Controller ctrl;
    stub.PhoneLogin(&ctrl, &req, &rsp, nullptr);
    if (ctrl.Failed() || !rsp.success()) {
      LOG_ERROR("{} {} 服务调用失败: {} {}", req.request_id(),
                _user_service_name, ctrl.ErrorText(), rsp.errmsg());
      err_rsp("服务调用失败");
      return;
    }

    response.set_content(rsp.SerializeAsString(), "application/protobuf");
  }

  void GetUserInfo(const httplib::Request& request,
                   httplib::Response& response) {
    GetUserInfoReq req;
    GetUserInfoRsp rsp;
    auto err_rsp = [&req, &rsp, &response](const std::string& errmsg) {
      rsp.set_success(false);
      rsp.set_errmsg(errmsg);
      response.set_content(rsp.SerializeAsString(), "application/protobuf");
    };

    bool ret = req.ParseFromString(request.body);
    if (!ret) {
      LOG_ERROR("请求正文反序列化失败");
      err_rsp("请求正文反序列化失败");
      return;
    }

    std::string login_session_id = req.login_session_id();
    auto user_id = _redis_session->user_id(login_session_id);
    if (!user_id) {
      LOG_ERROR("登录会话不存在");
      err_rsp("登录会话不存在");
      return;
    }
    req.set_user_id(*user_id);

    auto channel = _channels->get(_user_service_name);
    if (!channel) {
      LOG_ERROR("{} 服务节点不存在", _user_service_name);
      err_rsp("服务节点不存在");
      return;
    }

    huzch::UserService_Stub stub(channel.get());
    brpc::Controller ctrl;
    stub.GetUserInfo(&ctrl, &req, &rsp, nullptr);
    if (ctrl.Failed() || !rsp.success()) {
      LOG_ERROR("{} {} 服务调用失败: {} {}", req.request_id(),
                _user_service_name, ctrl.ErrorText(), rsp.errmsg());
      err_rsp("服务调用失败");
      return;
    }

    response.set_content(rsp.SerializeAsString(), "application/protobuf");
  }

  void UserSearch(const httplib::Request& request,
                  httplib::Response& response) {
    UserSearchReq req;
    UserSearchRsp rsp;
    auto err_rsp = [&req, &rsp, &response](const std::string& errmsg) {
      rsp.set_success(false);
      rsp.set_errmsg(errmsg);
      response.set_content(rsp.SerializeAsString(), "application/protobuf");
    };

    bool ret = req.ParseFromString(request.body);
    if (!ret) {
      LOG_ERROR("请求正文反序列化失败");
      err_rsp("请求正文反序列化失败");
      return;
    }

    std::string login_session_id = req.login_session_id();
    auto user_id = _redis_session->user_id(login_session_id);
    if (!user_id) {
      LOG_ERROR("登录会话不存在");
      err_rsp("登录会话不存在");
      return;
    }
    req.set_user_id(*user_id);

    auto channel = _channels->get(_user_service_name);
    if (!channel) {
      LOG_ERROR("{} 服务节点不存在", _user_service_name);
      err_rsp("服务节点不存在");
      return;
    }

    huzch::UserService_Stub stub(channel.get());
    brpc::Controller ctrl;
    stub.UserSearch(&ctrl, &req, &rsp, nullptr);
    if (ctrl.Failed() || !rsp.success()) {
      LOG_ERROR("{} {} 服务调用失败: {} {}", req.request_id(),
                _user_service_name, ctrl.ErrorText(), rsp.errmsg());
      err_rsp("服务调用失败");
      return;
    }

    response.set_content(rsp.SerializeAsString(), "application/protobuf");
  }

  void SetUserAvatar(const httplib::Request& request,
                     httplib::Response& response) {
    SetUserAvatarReq req;
    SetUserAvatarRsp rsp;
    auto err_rsp = [&req, &rsp, &response](const std::string& errmsg) {
      rsp.set_success(false);
      rsp.set_errmsg(errmsg);
      response.set_content(rsp.SerializeAsString(), "application/protobuf");
    };

    bool ret = req.ParseFromString(request.body);
    if (!ret) {
      LOG_ERROR("请求正文反序列化失败");
      err_rsp("请求正文反序列化失败");
      return;
    }

    std::string login_session_id = req.login_session_id();
    auto user_id = _redis_session->user_id(login_session_id);
    if (!user_id) {
      LOG_ERROR("登录会话不存在");
      err_rsp("登录会话不存在");
      return;
    }
    req.set_user_id(*user_id);

    auto channel = _channels->get(_user_service_name);
    if (!channel) {
      LOG_ERROR("{} 服务节点不存在", _user_service_name);
      err_rsp("服务节点不存在");
      return;
    }

    huzch::UserService_Stub stub(channel.get());
    brpc::Controller ctrl;
    stub.SetUserAvatar(&ctrl, &req, &rsp, nullptr);
    if (ctrl.Failed() || !rsp.success()) {
      LOG_ERROR("{} {} 服务调用失败: {} {}", req.request_id(),
                _user_service_name, ctrl.ErrorText(), rsp.errmsg());
      err_rsp("服务调用失败");
      return;
    }

    response.set_content(rsp.SerializeAsString(), "application/protobuf");
  }

  void SetUserName(const httplib::Request& request,
                   httplib::Response& response) {
    SetUserNameReq req;
    SetUserNameRsp rsp;
    auto err_rsp = [&req, &rsp, &response](const std::string& errmsg) {
      rsp.set_success(false);
      rsp.set_errmsg(errmsg);
      response.set_content(rsp.SerializeAsString(), "application/protobuf");
    };

    bool ret = req.ParseFromString(request.body);
    if (!ret) {
      LOG_ERROR("请求正文反序列化失败");
      err_rsp("请求正文反序列化失败");
      return;
    }

    std::string login_session_id = req.login_session_id();
    auto user_id = _redis_session->user_id(login_session_id);
    if (!user_id) {
      LOG_ERROR("登录会话不存在");
      err_rsp("登录会话不存在");
      return;
    }
    req.set_user_id(*user_id);

    auto channel = _channels->get(_user_service_name);
    if (!channel) {
      LOG_ERROR("{} 服务节点不存在", _user_service_name);
      err_rsp("服务节点不存在");
      return;
    }

    huzch::UserService_Stub stub(channel.get());
    brpc::Controller ctrl;
    stub.SetUserName(&ctrl, &req, &rsp, nullptr);
    if (ctrl.Failed() || !rsp.success()) {
      LOG_ERROR("{} {} 服务调用失败: {} {}", req.request_id(),
                _user_service_name, ctrl.ErrorText(), rsp.errmsg());
      err_rsp("服务调用失败");
      return;
    }

    response.set_content(rsp.SerializeAsString(), "application/protobuf");
  }

  void SetUserDescription(const httplib::Request& request,
                          httplib::Response& response) {
    SetUserDescriptionReq req;
    SetUserDescriptionRsp rsp;
    auto err_rsp = [&req, &rsp, &response](const std::string& errmsg) {
      rsp.set_success(false);
      rsp.set_errmsg(errmsg);
      response.set_content(rsp.SerializeAsString(), "application/protobuf");
    };

    bool ret = req.ParseFromString(request.body);
    if (!ret) {
      LOG_ERROR("请求正文反序列化失败");
      err_rsp("请求正文反序列化失败");
      return;
    }

    std::string login_session_id = req.login_session_id();
    auto user_id = _redis_session->user_id(login_session_id);
    if (!user_id) {
      LOG_ERROR("登录会话不存在");
      err_rsp("登录会话不存在");
      return;
    }
    req.set_user_id(*user_id);

    auto channel = _channels->get(_user_service_name);
    if (!channel) {
      LOG_ERROR("{} 服务节点不存在", _user_service_name);
      err_rsp("服务节点不存在");
      return;
    }

    huzch::UserService_Stub stub(channel.get());
    brpc::Controller ctrl;
    stub.SetUserDescription(&ctrl, &req, &rsp, nullptr);
    if (ctrl.Failed() || !rsp.success()) {
      LOG_ERROR("{} {} 服务调用失败: {} {}", req.request_id(),
                _user_service_name, ctrl.ErrorText(), rsp.errmsg());
      err_rsp("服务调用失败");
      return;
    }

    response.set_content(rsp.SerializeAsString(), "application/protobuf");
  }

  void SetUserPhoneNumber(const httplib::Request& request,
                          httplib::Response& response) {
    SetUserPhoneNumberReq req;
    SetUserPhoneNumberRsp rsp;
    auto err_rsp = [&req, &rsp, &response](const std::string& errmsg) {
      rsp.set_success(false);
      rsp.set_errmsg(errmsg);
      response.set_content(rsp.SerializeAsString(), "application/protobuf");
    };

    bool ret = req.ParseFromString(request.body);
    if (!ret) {
      LOG_ERROR("请求正文反序列化失败");
      err_rsp("请求正文反序列化失败");
      return;
    }

    std::string login_session_id = req.login_session_id();
    auto user_id = _redis_session->user_id(login_session_id);
    if (!user_id) {
      LOG_ERROR("登录会话不存在");
      err_rsp("登录会话不存在");
      return;
    }
    req.set_user_id(*user_id);

    auto channel = _channels->get(_user_service_name);
    if (!channel) {
      LOG_ERROR("{} 服务节点不存在", _user_service_name);
      err_rsp("服务节点不存在");
      return;
    }

    huzch::UserService_Stub stub(channel.get());
    brpc::Controller ctrl;
    stub.SetUserPhoneNumber(&ctrl, &req, &rsp, nullptr);
    if (ctrl.Failed() || !rsp.success()) {
      LOG_ERROR("{} {} 服务调用失败: {} {}", req.request_id(),
                _user_service_name, ctrl.ErrorText(), rsp.errmsg());
      err_rsp("服务调用失败");
      return;
    }

    response.set_content(rsp.SerializeAsString(), "application/protobuf");
  }

  void NewMessage(const httplib::Request& request,
                  httplib::Response& response) {
    NewMessageReq req;
    NewMessageRsp rsp;
    auto err_rsp = [&req, &rsp, &response](const std::string& errmsg) {
      rsp.set_success(false);
      rsp.set_errmsg(errmsg);
      response.set_content(rsp.SerializeAsString(), "application/protobuf");
    };

    bool ret = req.ParseFromString(request.body);
    if (!ret) {
      LOG_ERROR("请求正文反序列化失败");
      err_rsp("请求正文反序列化失败");
      return;
    }

    std::string login_session_id = req.login_session_id();
    auto user_id = _redis_session->user_id(login_session_id);
    if (!user_id) {
      LOG_ERROR("登录会话不存在");
      err_rsp("登录会话不存在");
      return;
    }
    req.set_user_id(*user_id);

    auto channel = _channels->get(_forward_service_name);
    if (!channel) {
      LOG_ERROR("{} 服务节点不存在", _forward_service_name);
      err_rsp("服务节点不存在");
      return;
    }

    huzch::ForwardService_Stub stub(channel.get());
    brpc::Controller ctrl;
    stub.NewMessage(&ctrl, &req, &rsp, nullptr);
    if (ctrl.Failed() || !rsp.success()) {
      LOG_ERROR("{} {} 服务调用失败: {} {}", req.request_id(),
                _forward_service_name, ctrl.ErrorText(), rsp.errmsg());
      err_rsp("服务调用失败");
      return;
    }

    // 发送消息后，对聊天会话中的成员进行通知
    NotifyMessage notify;
    notify.set_notify_type(NotifyType::CHAT_MESSAGE_NOTIFY);
    notify.mutable_new_message_info()->mutable_message_info()->CopyFrom(
        rsp.message_info());

    for (auto& target_id : rsp.targets_id()) {
      if (target_id == req.user_id()) {
        continue;
      }
      auto connection = _connections->get(target_id);
      if (connection) {
        connection->send(notify.SerializeAsString(),
                         websocketpp::frame::opcode::value::binary);
      }
    }
    rsp.clear_message_info();
    rsp.clear_targets_id();

    response.set_content(rsp.SerializeAsString(), "application/protobuf");
  }

  void GetHistoryMessage(const httplib::Request& request,
                         httplib::Response& response) {
    GetHistoryMessageReq req;
    GetHistoryMessageRsp rsp;
    auto err_rsp = [&req, &rsp, &response](const std::string& errmsg) {
      rsp.set_success(false);
      rsp.set_errmsg(errmsg);
      response.set_content(rsp.SerializeAsString(), "application/protobuf");
    };

    bool ret = req.ParseFromString(request.body);
    if (!ret) {
      LOG_ERROR("请求正文反序列化失败");
      err_rsp("请求正文反序列化失败");
      return;
    }

    std::string login_session_id = req.login_session_id();
    auto user_id = _redis_session->user_id(login_session_id);
    if (!user_id) {
      LOG_ERROR("登录会话不存在");
      err_rsp("登录会话不存在");
      return;
    }
    req.set_user_id(*user_id);

    auto channel = _channels->get(_message_service_name);
    if (!channel) {
      LOG_ERROR("{} 服务节点不存在", _message_service_name);
      err_rsp("服务节点不存在");
      return;
    }

    huzch::MessageService_Stub stub(channel.get());
    brpc::Controller ctrl;
    stub.GetHistoryMessage(&ctrl, &req, &rsp, nullptr);
    if (ctrl.Failed() || !rsp.success()) {
      LOG_ERROR("{} {} 服务调用失败: {} {}", req.request_id(),
                _message_service_name, ctrl.ErrorText(), rsp.errmsg());
      err_rsp("服务调用失败");
      return;
    }

    response.set_content(rsp.SerializeAsString(), "application/protobuf");
  }

  void GetRecentMessage(const httplib::Request& request,
                        httplib::Response& response) {
    GetRecentMessageReq req;
    GetRecentMessageRsp rsp;
    auto err_rsp = [&req, &rsp, &response](const std::string& errmsg) {
      rsp.set_success(false);
      rsp.set_errmsg(errmsg);
      response.set_content(rsp.SerializeAsString(), "application/protobuf");
    };

    bool ret = req.ParseFromString(request.body);
    if (!ret) {
      LOG_ERROR("请求正文反序列化失败");
      err_rsp("请求正文反序列化失败");
      return;
    }

    std::string login_session_id = req.login_session_id();
    auto user_id = _redis_session->user_id(login_session_id);
    if (!user_id) {
      LOG_ERROR("登录会话不存在");
      err_rsp("登录会话不存在");
      return;
    }
    req.set_user_id(*user_id);

    auto channel = _channels->get(_message_service_name);
    if (!channel) {
      LOG_ERROR("{} 服务节点不存在", _message_service_name);
      err_rsp("服务节点不存在");
      return;
    }

    huzch::MessageService_Stub stub(channel.get());
    brpc::Controller ctrl;
    stub.GetRecentMessage(&ctrl, &req, &rsp, nullptr);
    if (ctrl.Failed() || !rsp.success()) {
      LOG_ERROR("{} {} 服务调用失败: {} {}", req.request_id(),
                _message_service_name, ctrl.ErrorText(), rsp.errmsg());
      err_rsp("服务调用失败");
      return;
    }

    response.set_content(rsp.SerializeAsString(), "application/protobuf");
  }

  void MessageSearch(const httplib::Request& request,
                     httplib::Response& response) {
    MessageSearchReq req;
    MessageSearchRsp rsp;
    auto err_rsp = [&req, &rsp, &response](const std::string& errmsg) {
      rsp.set_success(false);
      rsp.set_errmsg(errmsg);
      response.set_content(rsp.SerializeAsString(), "application/protobuf");
    };

    bool ret = req.ParseFromString(request.body);
    if (!ret) {
      LOG_ERROR("请求正文反序列化失败");
      err_rsp("请求正文反序列化失败");
      return;
    }

    std::string login_session_id = req.login_session_id();
    auto user_id = _redis_session->user_id(login_session_id);
    if (!user_id) {
      LOG_ERROR("登录会话不存在");
      err_rsp("登录会话不存在");
      return;
    }
    req.set_user_id(*user_id);

    auto channel = _channels->get(_message_service_name);
    if (!channel) {
      LOG_ERROR("{} 服务节点不存在", _message_service_name);
      err_rsp("服务节点不存在");
      return;
    }

    huzch::MessageService_Stub stub(channel.get());
    brpc::Controller ctrl;
    stub.MessageSearch(&ctrl, &req, &rsp, nullptr);
    if (ctrl.Failed() || !rsp.success()) {
      LOG_ERROR("{} {} 服务调用失败: {} {}", req.request_id(),
                _message_service_name, ctrl.ErrorText(), rsp.errmsg());
      err_rsp("服务调用失败");
      return;
    }

    response.set_content(rsp.SerializeAsString(), "application/protobuf");
  }

  void GetFriend(const httplib::Request& request, httplib::Response& response) {
    GetFriendReq req;
    GetFriendRsp rsp;
    auto err_rsp = [&req, &rsp, &response](const std::string& errmsg) {
      rsp.set_success(false);
      rsp.set_errmsg(errmsg);
      response.set_content(rsp.SerializeAsString(), "application/protobuf");
    };

    bool ret = req.ParseFromString(request.body);
    if (!ret) {
      LOG_ERROR("请求正文反序列化失败");
      err_rsp("请求正文反序列化失败");
      return;
    }

    std::string login_session_id = req.login_session_id();
    auto user_id = _redis_session->user_id(login_session_id);
    if (!user_id) {
      LOG_ERROR("登录会话不存在");
      err_rsp("登录会话不存在");
      return;
    }
    req.set_user_id(*user_id);

    auto channel = _channels->get(_friend_service_name);
    if (!channel) {
      LOG_ERROR("{} 服务节点不存在", _friend_service_name);
      err_rsp("服务节点不存在");
      return;
    }

    huzch::FriendService_Stub stub(channel.get());
    brpc::Controller ctrl;
    stub.GetFriend(&ctrl, &req, &rsp, nullptr);
    if (ctrl.Failed() || !rsp.success()) {
      LOG_ERROR("{} {} 服务调用失败: {} {}", req.request_id(),
                _friend_service_name, ctrl.ErrorText(), rsp.errmsg());
      err_rsp("服务调用失败");
      return;
    }

    response.set_content(rsp.SerializeAsString(), "application/protobuf");
  }

  void FriendRemove(const httplib::Request& request,
                    httplib::Response& response) {
    FriendRemoveReq req;
    FriendRemoveRsp rsp;
    auto err_rsp = [&req, &rsp, &response](const std::string& errmsg) {
      rsp.set_success(false);
      rsp.set_errmsg(errmsg);
      response.set_content(rsp.SerializeAsString(), "application/protobuf");
    };

    bool ret = req.ParseFromString(request.body);
    if (!ret) {
      LOG_ERROR("请求正文反序列化失败");
      err_rsp("请求正文反序列化失败");
      return;
    }

    std::string login_session_id = req.login_session_id();
    auto user_id = _redis_session->user_id(login_session_id);
    if (!user_id) {
      LOG_ERROR("登录会话不存在");
      err_rsp("登录会话不存在");
      return;
    }
    req.set_user_id(*user_id);

    auto channel = _channels->get(_friend_service_name);
    if (!channel) {
      LOG_ERROR("{} 服务节点不存在", _friend_service_name);
      err_rsp("服务节点不存在");
      return;
    }

    huzch::FriendService_Stub stub(channel.get());
    brpc::Controller ctrl;
    stub.FriendRemove(&ctrl, &req, &rsp, nullptr);
    if (ctrl.Failed() || !rsp.success()) {
      LOG_ERROR("{} {} 服务调用失败: {} {}", req.request_id(),
                _friend_service_name, ctrl.ErrorText(), rsp.errmsg());
      err_rsp("服务调用失败");
      return;
    }

    // 好友删除后，向被删除人进行通知
    auto connection = _connections->get(req.peer_id());
    if (connection) {
      NotifyMessage notify;
      notify.set_notify_type(NotifyType::FRIEND_REMOVE_NOTIFY);
      notify.mutable_friend_remove()->set_user_id(req.user_id());
      connection->send(notify.SerializeAsString(),
                       websocketpp::frame::opcode::value::binary);
      LOG_INFO("向被删除人 {} 进行好友删除通知", req.peer_id());
    }

    response.set_content(rsp.SerializeAsString(), "application/protobuf");
  }

  void FriendAddSend(const httplib::Request& request,
                     httplib::Response& response) {
    FriendAddSendReq req;
    FriendAddSendRsp rsp;
    auto err_rsp = [&req, &rsp, &response](const std::string& errmsg) {
      rsp.set_success(false);
      rsp.set_errmsg(errmsg);
      response.set_content(rsp.SerializeAsString(), "application/protobuf");
    };

    bool ret = req.ParseFromString(request.body);
    if (!ret) {
      LOG_ERROR("请求正文反序列化失败");
      err_rsp("请求正文反序列化失败");
      return;
    }

    std::string login_session_id = req.login_session_id();
    auto user_id = _redis_session->user_id(login_session_id);
    if (!user_id) {
      LOG_ERROR("登录会话不存在");
      err_rsp("登录会话不存在");
      return;
    }
    req.set_user_id(*user_id);

    auto channel = _channels->get(_friend_service_name);
    if (!channel) {
      LOG_ERROR("{} 服务节点不存在", _friend_service_name);
      err_rsp("服务节点不存在");
      return;
    }

    huzch::FriendService_Stub stub(channel.get());
    brpc::Controller ctrl;
    stub.FriendAddSend(&ctrl, &req, &rsp, nullptr);
    if (ctrl.Failed() || !rsp.success()) {
      LOG_ERROR("{} {} 服务调用失败: {} {}", req.request_id(),
                _friend_service_name, ctrl.ErrorText(), rsp.errmsg());
      err_rsp("服务调用失败");
      return;
    }

    // 好友申请发送后，向被申请人进行通知
    auto process_connection = _connections->get(req.respondent_id());
    if (process_connection) {
      NotifyMessage notify;
      notify.set_notify_type(NotifyType::FRIEND_ADD_SEND_NOTIFY);

      UserInfo requester_info;
      ret = get_user(req.request_id(), req.user_id(), requester_info);
      if (!ret) {
        LOG_ERROR("{} 获取用户信息失败", req.request_id());
        err_rsp("获取用户信息失败");
        return;
      }
      notify.mutable_friend_add_send()->mutable_user_info()->CopyFrom(
          requester_info);
      process_connection->send(notify.SerializeAsString(),
                               websocketpp::frame::opcode::value::binary);
      LOG_INFO("向被申请人 {} 进行好友申请发送通知", req.respondent_id());
    }

    response.set_content(rsp.SerializeAsString(), "application/protobuf");
  }

  void FriendAddProcess(const httplib::Request& request,
                        httplib::Response& response) {
    FriendAddProcessReq req;
    FriendAddProcessRsp rsp;
    auto err_rsp = [&req, &rsp, &response](const std::string& errmsg) {
      rsp.set_success(false);
      rsp.set_errmsg(errmsg);
      response.set_content(rsp.SerializeAsString(), "application/protobuf");
    };

    bool ret = req.ParseFromString(request.body);
    if (!ret) {
      LOG_ERROR("请求正文反序列化失败");
      err_rsp("请求正文反序列化失败");
      return;
    }

    std::string login_session_id = req.login_session_id();
    auto user_id = _redis_session->user_id(login_session_id);
    if (!user_id) {
      LOG_ERROR("登录会话不存在");
      err_rsp("登录会话不存在");
      return;
    }
    req.set_user_id(*user_id);

    auto channel = _channels->get(_friend_service_name);
    if (!channel) {
      LOG_ERROR("{} 服务节点不存在", _friend_service_name);
      err_rsp("服务节点不存在");
      return;
    }

    huzch::FriendService_Stub stub(channel.get());
    brpc::Controller ctrl;
    stub.FriendAddProcess(&ctrl, &req, &rsp, nullptr);
    if (ctrl.Failed() || !rsp.success()) {
      LOG_ERROR("{} {} 服务调用失败: {} {}", req.request_id(),
                _friend_service_name, ctrl.ErrorText(), rsp.errmsg());
      err_rsp("服务调用失败");
      return;
    }

    UserInfo requester_info;
    ret = get_user(req.request_id(), req.requester_id(), requester_info);
    if (!ret) {
      LOG_ERROR("{} 获取用户信息失败", req.request_id());
      err_rsp("获取用户信息失败");
      return;
    }

    UserInfo respondent_info;
    ret = get_user(req.request_id(), req.user_id(), respondent_info);
    if (!ret) {
      LOG_ERROR("{} 获取用户信息失败", req.request_id());
      err_rsp("获取用户信息失败");
      return;
    }

    // 好友申请处理后，向申请人进行通知
    auto send_connection = _connections->get(req.requester_id());
    if (send_connection) {
      NotifyMessage notify;
      notify.set_notify_type(NotifyType::FRIEND_ADD_PROCESS_NOTIFY);
      notify.mutable_friend_add_process()->set_agree(req.agree());
      notify.mutable_friend_add_process()->mutable_user_info()->CopyFrom(
          respondent_info);
      send_connection->send(notify.SerializeAsString(),
                            websocketpp::frame::opcode::value::binary);
      LOG_INFO("向申请人 {} 进行好友申请处理通知", req.requester_id());
    }

    // 好友申请通过，则创建单聊会话，向双方进行通知
    if (req.agree()) {
      if (send_connection) {
        NotifyMessage notify;
        notify.set_notify_type(NotifyType::CHAT_SESSION_CREATE_NOTIFY);
        auto chat_session_info =
            notify.mutable_new_chat_session_info()->mutable_chat_session_info();
        chat_session_info->set_single_chat_friend_id(req.user_id());
        chat_session_info->set_chat_session_id(rsp.chat_session_id());
        chat_session_info->set_chat_session_name(respondent_info.name());
        chat_session_info->set_avatar(respondent_info.avatar());
        send_connection->send(notify.SerializeAsString(),
                              websocketpp::frame::opcode::value::binary);
        LOG_INFO("向申请人 {} 进行会话创建通知", req.requester_id());
      }

      auto process_connection = _connections->get(req.user_id());
      if (process_connection) {
        NotifyMessage notify;
        notify.set_notify_type(NotifyType::CHAT_SESSION_CREATE_NOTIFY);
        auto chat_session_info =
            notify.mutable_new_chat_session_info()->mutable_chat_session_info();
        chat_session_info->set_single_chat_friend_id(req.requester_id());
        chat_session_info->set_chat_session_id(rsp.chat_session_id());
        chat_session_info->set_chat_session_name(requester_info.name());
        chat_session_info->set_avatar(requester_info.avatar());
        process_connection->send(notify.SerializeAsString(),
                                 websocketpp::frame::opcode::value::binary);
        LOG_INFO("向被申请人 {} 进行会话创建通知", req.user_id());
      }
    }

    response.set_content(rsp.SerializeAsString(), "application/protobuf");
  }

  void GetRequester(const httplib::Request& request,
                    httplib::Response& response) {
    GetRequesterReq req;
    GetRequesterRsp rsp;
    auto err_rsp = [&req, &rsp, &response](const std::string& errmsg) {
      rsp.set_success(false);
      rsp.set_errmsg(errmsg);
      response.set_content(rsp.SerializeAsString(), "application/protobuf");
    };

    bool ret = req.ParseFromString(request.body);
    if (!ret) {
      LOG_ERROR("请求正文反序列化失败");
      err_rsp("请求正文反序列化失败");
      return;
    }

    std::string login_session_id = req.login_session_id();
    auto user_id = _redis_session->user_id(login_session_id);
    if (!user_id) {
      LOG_ERROR("登录会话不存在");
      err_rsp("登录会话不存在");
      return;
    }
    req.set_user_id(*user_id);

    auto channel = _channels->get(_friend_service_name);
    if (!channel) {
      LOG_ERROR("{} 服务节点不存在", _friend_service_name);
      err_rsp("服务节点不存在");
      return;
    }

    huzch::FriendService_Stub stub(channel.get());
    brpc::Controller ctrl;
    stub.GetRequester(&ctrl, &req, &rsp, nullptr);
    if (ctrl.Failed() || !rsp.success()) {
      LOG_ERROR("{} {} 服务调用失败: {} {}", req.request_id(),
                _friend_service_name, ctrl.ErrorText(), rsp.errmsg());
      err_rsp("服务调用失败");
      return;
    }

    response.set_content(rsp.SerializeAsString(), "application/protobuf");
  }

  void GetChatSession(const httplib::Request& request,
                      httplib::Response& response) {
    GetChatSessionReq req;
    GetChatSessionRsp rsp;
    auto err_rsp = [&req, &rsp, &response](const std::string& errmsg) {
      rsp.set_success(false);
      rsp.set_errmsg(errmsg);
      response.set_content(rsp.SerializeAsString(), "application/protobuf");
    };

    bool ret = req.ParseFromString(request.body);
    if (!ret) {
      LOG_ERROR("请求正文反序列化失败");
      err_rsp("请求正文反序列化失败");
      return;
    }

    std::string login_session_id = req.login_session_id();
    auto user_id = _redis_session->user_id(login_session_id);
    if (!user_id) {
      LOG_ERROR("登录会话不存在");
      err_rsp("登录会话不存在");
      return;
    }
    req.set_user_id(*user_id);

    auto channel = _channels->get(_friend_service_name);
    if (!channel) {
      LOG_ERROR("{} 服务节点不存在", _friend_service_name);
      err_rsp("服务节点不存在");
      return;
    }

    huzch::FriendService_Stub stub(channel.get());
    brpc::Controller ctrl;
    stub.GetChatSession(&ctrl, &req, &rsp, nullptr);
    if (ctrl.Failed() || !rsp.success()) {
      LOG_ERROR("{} {} 服务调用失败: {} {}", req.request_id(),
                _friend_service_name, ctrl.ErrorText(), rsp.errmsg());
      err_rsp("服务调用失败");
      return;
    }

    response.set_content(rsp.SerializeAsString(), "application/protobuf");
  }

  void ChatSessionCreate(const httplib::Request& request,
                         httplib::Response& response) {
    ChatSessionCreateReq req;
    ChatSessionCreateRsp rsp;
    auto err_rsp = [&req, &rsp, &response](const std::string& errmsg) {
      rsp.set_success(false);
      rsp.set_errmsg(errmsg);
      response.set_content(rsp.SerializeAsString(), "application/protobuf");
    };

    bool ret = req.ParseFromString(request.body);
    if (!ret) {
      LOG_ERROR("请求正文反序列化失败");
      err_rsp("请求正文反序列化失败");
      return;
    }

    std::string login_session_id = req.login_session_id();
    auto user_id = _redis_session->user_id(login_session_id);
    if (!user_id) {
      LOG_ERROR("登录会话不存在");
      err_rsp("登录会话不存在");
      return;
    }
    req.set_user_id(*user_id);

    auto channel = _channels->get(_friend_service_name);
    if (!channel) {
      LOG_ERROR("{} 服务节点不存在", _friend_service_name);
      err_rsp("服务节点不存在");
      return;
    }

    huzch::FriendService_Stub stub(channel.get());
    brpc::Controller ctrl;
    stub.ChatSessionCreate(&ctrl, &req, &rsp, nullptr);
    if (ctrl.Failed() || !rsp.success()) {
      LOG_ERROR("{} {} 服务调用失败: {} {}", req.request_id(),
                _friend_service_name, ctrl.ErrorText(), rsp.errmsg());
      err_rsp("服务调用失败");
      return;
    }

    // 创建群聊后，向群聊成员进行通知
    NotifyMessage notify;
    notify.set_notify_type(NotifyType::CHAT_SESSION_CREATE_NOTIFY);
    notify.mutable_new_chat_session_info()
        ->mutable_chat_session_info()
        ->CopyFrom(rsp.chat_session_info());

    for (auto& member_id : req.members_id()) {
      auto connection = _connections->get(member_id);
      if (connection) {
        connection->send(notify.SerializeAsString(),
                         websocketpp::frame::opcode::value::binary);
        LOG_INFO("向群聊成员 {} 进行群聊创建通知", member_id);
      }
    }
    rsp.clear_chat_session_info();

    response.set_content(rsp.SerializeAsString(), "application/protobuf");
  }

  void GetChatSessionMember(const httplib::Request& request,
                            httplib::Response& response) {
    GetChatSessionMemberReq req;
    GetChatSessionMemberRsp rsp;
    auto err_rsp = [&req, &rsp, &response](const std::string& errmsg) {
      rsp.set_success(false);
      rsp.set_errmsg(errmsg);
      response.set_content(rsp.SerializeAsString(), "application/protobuf");
    };

    bool ret = req.ParseFromString(request.body);
    if (!ret) {
      LOG_ERROR("请求正文反序列化失败");
      err_rsp("请求正文反序列化失败");
      return;
    }

    std::string login_session_id = req.login_session_id();
    auto user_id = _redis_session->user_id(login_session_id);
    if (!user_id) {
      LOG_ERROR("登录会话不存在");
      err_rsp("登录会话不存在");
      return;
    }
    req.set_user_id(*user_id);

    auto channel = _channels->get(_friend_service_name);
    if (!channel) {
      LOG_ERROR("{} 服务节点不存在", _friend_service_name);
      err_rsp("服务节点不存在");
      return;
    }

    huzch::FriendService_Stub stub(channel.get());
    brpc::Controller ctrl;
    stub.GetChatSessionMember(&ctrl, &req, &rsp, nullptr);
    if (ctrl.Failed() || !rsp.success()) {
      LOG_ERROR("{} {} 服务调用失败: {} {}", req.request_id(),
                _friend_service_name, ctrl.ErrorText(), rsp.errmsg());
      err_rsp("服务调用失败");
      return;
    }

    response.set_content(rsp.SerializeAsString(), "application/protobuf");
  }

  bool get_user(const std::string& request_id, const std::string& user_id,
                UserInfo& user_info) {
    GetUserInfoReq req;
    GetUserInfoRsp rsp;
    auto err_rsp = [&rsp](const std::string& errmsg) {
      rsp.set_success(false);
      rsp.set_errmsg(errmsg);
    };
    req.set_request_id(request_id);
    req.set_user_id(user_id);

    auto channel = _channels->get(_user_service_name);
    if (!channel) {
      LOG_ERROR("{} 服务节点不存在", _user_service_name);
      err_rsp("服务节点不存在");
      return false;
    }

    huzch::UserService_Stub stub(channel.get());
    brpc::Controller ctrl;
    stub.GetUserInfo(&ctrl, &req, &rsp, nullptr);
    if (ctrl.Failed() || !rsp.success()) {
      LOG_ERROR("{} {} 服务调用失败: {} {}", req.request_id(),
                _user_service_name, ctrl.ErrorText(), rsp.errmsg());
      err_rsp("服务调用失败");
      return false;
    }

    user_info.CopyFrom(rsp.user_info());
    return true;
  }

 private:
  Session::Ptr _redis_session;
  Status::Ptr _redis_status;

  std::string _speech_service_name;
  std::string _file_service_name;
  std::string _user_service_name;
  std::string _forward_service_name;
  std::string _message_service_name;
  std::string _friend_service_name;
  ChannelManager::Ptr _channels;

  ConnectionManager::Ptr _connections;

  websocketpp::server<websocketpp::config::asio> _websocket_server;
  httplib::Server _http_server;
  std::thread _http_thread;
};

class GatewayServerBuilder {
 public:
  void init_discovery_client(const std::string& registry_host,
                             const std::string& base_dir,
                             const std::string& speech_service_name,
                             const std::string& file_service_name,
                             const std::string& user_service_name,
                             const std::string& forward_service_name,
                             const std::string& message_service_name,
                             const std::string& friend_service_name) {
    _speech_service_name = base_dir + speech_service_name;
    _file_service_name = base_dir + file_service_name;
    _user_service_name = base_dir + user_service_name;
    _forward_service_name = base_dir + forward_service_name;
    _message_service_name = base_dir + message_service_name;
    _friend_service_name = base_dir + friend_service_name;

    _channels = std::make_shared<ChannelManager>();
    _channels->declare(base_dir + speech_service_name);
    _channels->declare(base_dir + file_service_name);
    _channels->declare(base_dir + user_service_name);
    _channels->declare(base_dir + forward_service_name);
    _channels->declare(base_dir + message_service_name);
    _channels->declare(base_dir + friend_service_name);
    auto put_cb = std::bind(&ChannelManager::on_service_online, _channels.get(),
                            std::placeholders::_1, std::placeholders::_2);
    auto del_cb =
        std::bind(&ChannelManager::on_service_offline, _channels.get(),
                  std::placeholders::_1, std::placeholders::_2);

    _discovery_client = std::make_shared<ServiceDiscovery>(
        registry_host, base_dir, put_cb, del_cb);
  }

  void init_redis_client(const std::string& host, int port, int db,
                         bool keep_alive) {
    _redis_client = RedisClientFactory::create(host, port, db, keep_alive);
  }

  void init_http_websocket_server(int http_port, int websocket_port) {
    _http_port = http_port;
    _websocket_port = websocket_port;
  }

  GatewayServer::Ptr build() {
    if (!_redis_client) {
      LOG_ERROR("未初始化redis数据库模块");
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

    return std::make_shared<GatewayServer>(
        _http_port, _websocket_port, _redis_client, _speech_service_name,
        _file_service_name, _user_service_name, _forward_service_name,
        _message_service_name, _friend_service_name, _channels);
  }

 private:
  int _http_port;
  int _websocket_port;

  ServiceDiscovery::Ptr _discovery_client;
  std::shared_ptr<sw::redis::Redis> _redis_client;

  std::string _speech_service_name;
  std::string _file_service_name;
  std::string _user_service_name;
  std::string _forward_service_name;
  std::string _message_service_name;
  std::string _friend_service_name;
  ChannelManager::Ptr _channels;
};

}  // namespace huzch