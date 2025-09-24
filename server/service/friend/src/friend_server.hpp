#pragma once
#include <brpc/server.h>

#include "base.pb.h"
#include "channel.hpp"
#include "data_mysql_friend_request.hpp"
#include "data_mysql_relation.hpp"
#include "data_mysql_session.hpp"
#include "data_mysql_session_member.hpp"
#include "data_search.hpp"
#include "etcd.hpp"
#include "friend.pb.h"
#include "message.pb.h"
#include "user.pb.h"
#include "utils.hpp"

namespace huzch {

class FriendServiceImpl : public FriendService {
 public:
  FriendServiceImpl(const std::shared_ptr<elasticlient::Client>& es_client,
                    const std::shared_ptr<odb::core::database>& mysql_client,
                    const std::string& user_service_name,
                    const std::string& message_service_name,
                    const ServiceManager::Ptr& service_manager)
      : _es_user(std::make_shared<ESUser>(es_client)),
        _mysql_session(std::make_shared<SessionTable>(mysql_client)),
        _mysql_session_member(
            std::make_shared<SessionMemberTable>(mysql_client)),
        _mysql_relation(std::make_shared<RelationTable>(mysql_client)),
        _mysql_friend_request(
            std::make_shared<FriendRequestTable>(mysql_client)),
        _user_service_name(user_service_name),
        _message_service_name(message_service_name),
        _service_manager(service_manager) {}

  void GetFriendList(google::protobuf::RpcController* controller,
                     const GetFriendListReq* request,
                     GetFriendListRsp* response,
                     google::protobuf::Closure* done) {
    brpc::ClosureGuard rpc_guard(done);
    std::string request_id = request->request_id();
    response->set_request_id(request_id);

    auto err_rsp = [response](const std::string& errmsg) {
      response->set_success(false);
      response->set_errmsg(errmsg);
    };
    std::string user_id = request->user_id();

    auto friends_id = _mysql_relation->friends_id(user_id);

    std::unordered_set<std::string> users_id;
    for (auto& friend_id : friends_id) {
      users_id.insert(friend_id);
    }
    std::unordered_map<std::string, UserInfo> users_info;
    bool ret = get_user(request_id, users_id, users_info);
    if (!ret) {
      LOG_ERROR("{} 批量获取用户信息失败", request_id);
      err_rsp("批量获取用户信息失败");
      return;
    }

    for (auto& [user_id, user_info] : users_info) {
      auto friend_info = response->add_friends_info();
      friend_info->CopyFrom(user_info);
    }
    response->set_success(true);
  }

  void FriendRemove(google::protobuf::RpcController* controller,
                    const FriendRemoveReq* request, FriendRemoveRsp* response,
                    google::protobuf::Closure* done) {
    brpc::ClosureGuard rpc_guard(done);
    std::string request_id = request->request_id();
    response->set_request_id(request_id);

    auto err_rsp = [response](const std::string& errmsg) {
      response->set_success(false);
      response->set_errmsg(errmsg);
    };
    std::string user_id = request->user_id();
    std::string peer_id = request->peer_id();
    std::string chat_session_id = request->chat_session_id();

    bool ret = _mysql_relation->remove(user_id, peer_id);
    if (!ret) {
      LOG_ERROR("{} mysql移除好友失败", request_id);
      err_rsp("mysql移除好友失败");
      return;
    }

    ret = _mysql_session->remove(chat_session_id);
    if (!ret) {
      LOG_ERROR("{} mysql移除好友会话失败", request_id);
      err_rsp("mysql移除好友会话失败");
      return;
    }

    response->set_success(true);
  }

  void FriendAddSend(google::protobuf::RpcController* controller,
                     const FriendAddSendReq* request,
                     FriendAddSendRsp* response,
                     google::protobuf::Closure* done) {
    brpc::ClosureGuard rpc_guard(done);
    std::string request_id = request->request_id();
    response->set_request_id(request_id);

    auto err_rsp = [response](const std::string& errmsg) {
      response->set_success(false);
      response->set_errmsg(errmsg);
    };
    std::string user_id = request->user_id();
    std::string respondent_id = request->respondent_id();

    bool ret = _mysql_relation->exists(user_id, respondent_id);
    if (ret) {
      LOG_ERROR("{} 用户 {} 已存在好友 {}", request_id, user_id, respondent_id);
      err_rsp("用户已存在好友");
      return;
    }

    ret = _mysql_friend_request->exists(user_id, respondent_id);
    if (ret) {
      LOG_ERROR("{} 用户 {} 已申请添加好友 {}", request_id, user_id,
                respondent_id);
      err_rsp("用户已申请添加好友");
      return;
    }

    FriendRequest friend_request(user_id, respondent_id);
    ret = _mysql_friend_request->insert(friend_request);
    if (!ret) {
      LOG_ERROR("{} mysql新增好友申请失败", request_id);
      err_rsp("mysql新增好友申请失败");
      return;
    }
    response->set_success(true);
  }

  void FriendAddProcess(google::protobuf::RpcController* controller,
                        const FriendAddProcessReq* request,
                        FriendAddProcessRsp* response,
                        google::protobuf::Closure* done) {
    brpc::ClosureGuard rpc_guard(done);
    std::string request_id = request->request_id();
    response->set_request_id(request_id);

    auto err_rsp = [response](const std::string& errmsg) {
      response->set_success(false);
      response->set_errmsg(errmsg);
    };
    bool agree = request->agree();
    std::string requester_id = request->requester_id();
    std::string user_id = request->user_id();

    bool ret = _mysql_friend_request->remove(requester_id, user_id);
    if (!ret) {
      LOG_ERROR("{} mysql移除好友申请失败", request_id);
      err_rsp("mysql移除好友申请失败");
      return;
    }

    if (!agree) {
      LOG_INFO("{} 好友申请被拒绝", request_id);
      response->set_success(true);
      return;
    }

    ret = _mysql_relation->insert(requester_id, user_id);
    if (!ret) {
      LOG_ERROR("{} mysql新增好友失败", request_id);
      err_rsp("mysql新增好友失败");
      return;
    }

    std::string chat_session_id = uuid();
    Session session(chat_session_id, "", SessionType::SINGLE);
    ret = _mysql_session->insert(session);
    if (!ret) {
      LOG_ERROR("{} mysql新增会话失败", request_id);
      err_rsp("mysql新增会话失败");
      return;
    }

    std::vector<SessionMember> members;
    members.emplace_back(chat_session_id, requester_id);
    members.emplace_back(chat_session_id, user_id);
    ret = _mysql_session_member->insert(members);
    if (!ret) {
      LOG_ERROR("{} mysql新增会话成员失败", request_id);
      err_rsp("mysql新增会话成员失败");
      return;
    }

    response->set_success(true);
    response->set_chat_session_id(chat_session_id);
  }

  void GetRequesterList(google::protobuf::RpcController* controller,
                        const GetRequesterListReq* request,
                        GetRequesterListRsp* response,
                        google::protobuf::Closure* done) {
    brpc::ClosureGuard rpc_guard(done);
    std::string request_id = request->request_id();
    response->set_request_id(request_id);

    auto err_rsp = [response](const std::string& errmsg) {
      response->set_success(false);
      response->set_errmsg(errmsg);
    };
    std::string user_id = request->user_id();

    auto requesters_id = _mysql_friend_request->requesters_id(user_id);

    std::unordered_set<std::string> users_id;
    for (auto& requester_id : requesters_id) {
      users_id.insert(requester_id);
    }
    std::unordered_map<std::string, UserInfo> users_info;
    bool ret = get_user(request_id, users_id, users_info);
    if (!ret) {
      LOG_ERROR("{} 批量获取用户信息失败", request_id);
      err_rsp("批量获取用户信息失败");
      return;
    }

    for (auto& [user_id, user_info] : users_info) {
      auto requester_info = response->add_requesters_info();
      requester_info->CopyFrom(user_info);
    }
    response->set_success(true);
  }

  void GetChatSessionList(google::protobuf::RpcController* controller,
                          const GetChatSessionListReq* request,
                          GetChatSessionListRsp* response,
                          google::protobuf::Closure* done) {
    brpc::ClosureGuard rpc_guard(done);
    std::string request_id = request->request_id();
    response->set_request_id(request_id);

    auto err_rsp = [response](const std::string& errmsg) {
      response->set_success(false);
      response->set_errmsg(errmsg);
    };
    std::string user_id = request->user_id();

    auto single_sessions = _mysql_session->single_sessions(user_id);

    std::unordered_set<std::string> users_id;
    for (auto& single_session : single_sessions) {
      users_id.insert(single_session._friend_id);
    }
    std::unordered_map<std::string, UserInfo> users_info;
    bool ret = get_user(request_id, users_id, users_info);
    if (!ret) {
      LOG_ERROR("{} 批量获取用户信息失败", request_id);
      err_rsp("批量获取用户信息失败");
      return;
    }

    for (auto& single_session : single_sessions) {
      auto chat_session_info = response->add_chat_sessions_info();
      chat_session_info->set_single_chat_friend_id(single_session._friend_id);
      chat_session_info->set_chat_session_id(single_session._session_id);
      chat_session_info->set_chat_session_name(
          users_info[single_session._friend_id].name());
      chat_session_info->set_avatar(
          users_info[single_session._friend_id].avatar());

      MessageInfo message_info;
      ret = get_message(request_id, single_session._session_id, message_info);
      if (!ret) {
        LOG_ERROR("{} 获取最近会话消息失败", request_id);
        continue;
      }
      chat_session_info->mutable_prev_message()->CopyFrom(message_info);
    }

    auto group_sessions = _mysql_session->group_sessions(user_id);

    for (auto& group_session : group_sessions) {
      auto chat_session_info = response->add_chat_sessions_info();
      chat_session_info->set_chat_session_id(group_session._session_id);
      chat_session_info->set_chat_session_name(group_session._session_name);

      MessageInfo message_info;
      ret = get_message(request_id, group_session._session_id, message_info);
      if (!ret) {
        LOG_ERROR("{} 获取最近会话消息失败", request_id);
        continue;
      }
      chat_session_info->mutable_prev_message()->CopyFrom(message_info);
    }

    response->set_success(true);
  }

  void ChatSessionCreate(google::protobuf::RpcController* controller,
                         const ChatSessionCreateReq* request,
                         ChatSessionCreateRsp* response,
                         google::protobuf::Closure* done) {
    brpc::ClosureGuard rpc_guard(done);
    std::string request_id = request->request_id();
    response->set_request_id(request_id);

    auto err_rsp = [response](const std::string& errmsg) {
      response->set_success(false);
      response->set_errmsg(errmsg);
    };
    std::string chat_session_name = request->chat_session_name();

    std::string chat_session_id = uuid();
    Session session(chat_session_id, chat_session_name, SessionType::GROUP);
    bool ret = _mysql_session->insert(session);
    if (!ret) {
      LOG_ERROR("{} mysql新增会话失败", request_id);
      err_rsp("mysql新增会话失败");
      return;
    }

    std::vector<SessionMember> members;
    members.reserve(request->members_id_size());
    for (auto& member_id : request->members_id()) {
      members.emplace_back(chat_session_id, member_id);
    }
    ret = _mysql_session_member->insert(members);
    if (!ret) {
      LOG_ERROR("{} mysql新增会话成员失败", request_id);
      err_rsp("mysql新增会话成员失败");
      return;
    }

    response->set_success(true);
    response->mutable_chat_session_info()->set_chat_session_id(chat_session_id);
    response->mutable_chat_session_info()->set_chat_session_name(
        chat_session_name);
  }

  void GetChatSessionMember(google::protobuf::RpcController* controller,
                            const GetChatSessionMemberReq* request,
                            GetChatSessionMemberRsp* response,
                            google::protobuf::Closure* done) {
    brpc::ClosureGuard rpc_guard(done);
    std::string request_id = request->request_id();
    response->set_request_id(request_id);

    auto err_rsp = [response](const std::string& errmsg) {
      response->set_success(false);
      response->set_errmsg(errmsg);
    };
    std::string chat_session_id = request->chat_session_id();

    auto members = _mysql_session_member->members(chat_session_id);

    std::unordered_set<std::string> users_id;
    for (auto& member : members) {
      users_id.insert(member.user_id());
    }
    std::unordered_map<std::string, UserInfo> users_info;
    bool ret = get_user(request_id, users_id, users_info);
    if (!ret) {
      LOG_ERROR("{} 批量获取用户信息失败", request_id);
      err_rsp("批量获取用户信息失败");
      return;
    }

    for (auto& [user_id, user_info] : users_info) {
      auto member_info = response->add_members_info();
      member_info->CopyFrom(user_info);
    }
    response->set_success(true);
  }

 private:
  bool get_user(const std::string& request_id,
                const std::unordered_set<std::string> users_id,
                std::unordered_map<std::string, UserInfo>& users_info) {
    auto channel = _service_manager->get(_user_service_name);
    if (!channel) {
      LOG_ERROR("{} 未找到 {} 服务节点", request_id, _user_service_name);
      return false;
    }

    huzch::UserService_Stub stub(channel.get());
    brpc::Controller ctrl;
    huzch::GetMultiUserInfoReq req;
    req.set_request_id(request_id);
    for (const auto& user_id : users_id) {
      req.add_users_id(user_id);
    }
    huzch::GetMultiUserInfoRsp rsp;

    stub.GetMultiUserInfo(&ctrl, &req, &rsp, nullptr);
    if (ctrl.Failed() || !rsp.success()) {
      LOG_ERROR("{} {} 服务调用失败: {} {}", request_id, _user_service_name,
                ctrl.ErrorText(), rsp.errmsg());
      return false;
    }

    for (const auto& [key, val] : rsp.users_info()) {
      users_info[key] = val;
    }
    return true;
  }

  bool get_message(const std::string& request_id,
                   const std::string& chat_session_id,
                   MessageInfo& message_info) {
    auto channel = _service_manager->get(_message_service_name);
    if (!channel) {
      LOG_ERROR("{} 未找到 {} 服务节点", request_id, _message_service_name);
      return false;
    }

    huzch::MessageService_Stub stub(channel.get());
    brpc::Controller ctrl;
    huzch::GetRecentMessageReq req;
    req.set_request_id(request_id);
    req.set_chat_session_id(chat_session_id);
    req.set_msg_count(1);
    huzch::GetRecentMessageRsp rsp;

    stub.GetRecentMessage(&ctrl, &req, &rsp, nullptr);
    if (ctrl.Failed() || !rsp.success()) {
      LOG_ERROR("{} {} 服务调用失败: {} {}", request_id, _user_service_name,
                ctrl.ErrorText(), rsp.errmsg());
      return false;
    }

    if (rsp.messages_info_size() == 0) {
      return false;
    }

    message_info = rsp.messages_info(0);
    return true;
  }

 private:
  ESUser::Ptr _es_user;
  SessionTable::Ptr _mysql_session;
  SessionMemberTable::Ptr _mysql_session_member;
  RelationTable::Ptr _mysql_relation;
  FriendRequestTable::Ptr _mysql_friend_request;

  std::string _user_service_name;
  std::string _message_service_name;
  ServiceManager::Ptr _service_manager;
};

class FriendServer {
 public:
  using Ptr = std::shared_ptr<FriendServer>;

 public:
  FriendServer(const std::shared_ptr<brpc::Server>& server) : _server(server) {}

  // 启动服务器
  void start() { _server->RunUntilAskedToQuit(); }

 private:
  std::shared_ptr<brpc::Server> _server;
};

class FriendServerBuilder {
 public:
  void init_registry_client(const std::string& registry_host,
                            const std::string& service_name,
                            const std::string& service_host) {
    _registry_client = std::make_shared<ServiceRegistry>(registry_host);
    _registry_client->register_service(service_name, service_host);
  }

  void init_discovery_client(const std::string& registry_host,
                             const std::string& base_dir,
                             const std::string& user_service_name,
                             const std::string& message_service_name) {
    _user_service_name = base_dir + user_service_name;
    _message_service_name = base_dir + message_service_name;
    _service_manager = std::make_shared<ServiceManager>();
    _service_manager->declare(base_dir + user_service_name);
    _service_manager->declare(base_dir + message_service_name);
    auto put_cb =
        std::bind(&ServiceManager::on_service_online, _service_manager.get(),
                  std::placeholders::_1, std::placeholders::_2);
    auto del_cb =
        std::bind(&ServiceManager::on_service_offline, _service_manager.get(),
                  std::placeholders::_1, std::placeholders::_2);

    _discovery_client = std::make_shared<ServiceDiscovery>(
        registry_host, base_dir, put_cb, del_cb);
  }

  void init_es_client(const std::vector<std::string>& host_list) {
    _es_client = ESClientFactory::create(host_list);
  }

  void init_mysql_client(const std::string& user, const std::string& passwd,
                         const std::string& db, const std::string& host,
                         size_t port, const std::string& charset,
                         size_t max_connections) {
    _mysql_client = MysqlClientFactory::create(user, passwd, db, host, port,
                                               charset, max_connections);
  }

  void init_rpc_server(int port, int timeout, int num_threads) {
    if (!_es_client) {
      LOG_ERROR("未初始化es搜索引擎模块");
      abort();
    }

    if (!_mysql_client) {
      LOG_ERROR("未初始化mysql数据库模块");
      abort();
    }

    _server = std::make_shared<brpc::Server>();
    auto friend_service =
        new FriendServiceImpl(_es_client, _mysql_client, _user_service_name,
                              _message_service_name, _service_manager);
    int ret = _server->AddService(friend_service,
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

  FriendServer::Ptr build() {
    if (!_registry_client) {
      LOG_ERROR("未初始化服务注册模块");
      abort();
    }

    if (!_discovery_client) {
      LOG_ERROR("未初始化服务发现模块");
      abort();
    }

    if (!_server) {
      LOG_ERROR("未初始化rpc服务器模块");
      abort();
    }

    auto server = std::make_shared<FriendServer>(_server);
    return server;
  }

 private:
  ServiceRegistry::Ptr _registry_client;
  ServiceDiscovery::Ptr _discovery_client;
  std::shared_ptr<elasticlient::Client> _es_client;
  std::shared_ptr<odb::core::database> _mysql_client;
  std::shared_ptr<brpc::Server> _server;

  std::string _user_service_name;
  std::string _message_service_name;
  ServiceManager::Ptr _service_manager;
};

}  // namespace huzch