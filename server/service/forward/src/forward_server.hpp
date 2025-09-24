#pragma once
#include <brpc/server.h>

#include "base.pb.h"
#include "channel.hpp"
#include "data_mysql_session_member.hpp"
#include "etcd.hpp"
#include "forward.pb.h"
#include "mq.hpp"
#include "user.pb.h"
#include "utils.hpp"

namespace huzch {

class ForwardServiceImpl : public ForwardService {
 public:
  ForwardServiceImpl(const std::shared_ptr<odb::core::database>& mysql_client,
                     const std::string& exchange_name,
                     const MQClient::Ptr& mq_client,
                     const std::string& user_service_name,
                     const ServiceManager::Ptr& service_manager)
      : _mysql_session_member(
            std::make_shared<SessionMemberTable>(mysql_client)),
        _exchange_name(exchange_name),
        _mq_client(mq_client),
        _user_service_name(user_service_name),
        _service_manager(service_manager) {}

  void GetForwardTarget(google::protobuf::RpcController* controller,
                        const NewMessageReq* request,
                        GetForwardTargetRsp* response,
                        google::protobuf::Closure* done) {
    brpc::ClosureGuard rpc_guard(done);
    std::string request_id = request->request_id();
    response->set_request_id(request_id);

    auto err_rsp = [response](const std::string& errmsg) {
      response->set_success(false);
      response->set_errmsg(errmsg);
    };
    std::string user_id = request->user_id();
    std::string chat_session_id = request->chat_session_id();
    const MessageContent& content = request->message();

    auto channel = _service_manager->get(_user_service_name);
    if (!channel) {
      LOG_ERROR("{} 未找到 {} 服务节点", request_id, _user_service_name);
      err_rsp("未找到服务节点");
      return;
    }

    huzch::UserService_Stub stub(channel.get());
    brpc::Controller ctrl;
    huzch::GetUserInfoReq req;
    req.set_request_id(request_id);
    req.set_user_id(user_id);
    huzch::GetUserInfoRsp rsp;

    stub.GetUserInfo(&ctrl, &req, &rsp, nullptr);
    if (ctrl.Failed() || !rsp.success()) {
      LOG_ERROR("{} {} 服务调用失败: {} {}", request_id, _user_service_name,
                ctrl.ErrorText(), rsp.errmsg());
      err_rsp("服务调用失败");
      return;
    }

    MessageInfo message_info;
    message_info.set_message_id(uuid());
    message_info.set_chat_session_id(chat_session_id);
    message_info.set_timestamp(time(nullptr));
    message_info.mutable_sender()->CopyFrom(rsp.user_info());
    message_info.mutable_message()->CopyFrom(content);

    auto members = _mysql_session_member->members(chat_session_id);

    // 将封装好的消息信息发布到消息队列，等待消息服务进行消息持久化
    bool ret =
        _mq_client->publish(_exchange_name, message_info.SerializeAsString());
    if (!ret) {
      LOG_ERROR("{} 持久化消息发布失败", request_id);
      err_rsp("持久化消息发布失败");
      return;
    }

    response->set_success(true);
    response->mutable_message()->CopyFrom(message_info);
    for (auto& member : members) {
      response->add_targets_id(member.user_id());
    }
  }

 private:
  SessionMemberTable::Ptr _mysql_session_member;

  std::string _exchange_name;
  MQClient::Ptr _mq_client;
  std::string _user_service_name;
  ServiceManager::Ptr _service_manager;
};

class ForwardServer {
 public:
  using Ptr = std::shared_ptr<ForwardServer>;

 public:
  ForwardServer(const std::shared_ptr<brpc::Server>& server)
      : _server(server) {}

  // 启动服务器
  void start() { _server->RunUntilAskedToQuit(); }

 private:
  std::shared_ptr<brpc::Server> _server;
};

class ForwardServerBuilder {
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
    _user_service_name = base_dir + service_name;
    _service_manager = std::make_shared<ServiceManager>();
    _service_manager->declare(base_dir + service_name);
    auto put_cb =
        std::bind(&ServiceManager::on_service_online, _service_manager.get(),
                  std::placeholders::_1, std::placeholders::_2);
    auto del_cb =
        std::bind(&ServiceManager::on_service_offline, _service_manager.get(),
                  std::placeholders::_1, std::placeholders::_2);

    _discovery_client = std::make_shared<ServiceDiscovery>(
        registry_host, base_dir, put_cb, del_cb);
  }

  void init_mq_client(const std::string& user, const std::string& passwd,
                      const std::string& host, const std::string& exchange,
                      const std::string& queue) {
    _exchange_name = exchange;
    _mq_client = std::make_shared<MQClient>(user, passwd, host);
    _mq_client->declare(exchange, queue);
  }

  void init_mysql_client(const std::string& user, const std::string& passwd,
                         const std::string& db, const std::string& host,
                         size_t port, const std::string& charset,
                         size_t max_connections) {
    _mysql_client = MysqlClientFactory::create(user, passwd, db, host, port,
                                               charset, max_connections);
  }

  void init_rpc_server(int port, int timeout, int num_threads) {
    if (!_mq_client) {
      LOG_ERROR("未初始化rabbitmq消息队列模块");
      abort();
    }

    if (!_mysql_client) {
      LOG_ERROR("未初始化mysql数据库模块");
      abort();
    }

    _server = std::make_shared<brpc::Server>();
    auto forward_service =
        new ForwardServiceImpl(_mysql_client, _exchange_name, _mq_client,
                               _user_service_name, _service_manager);
    int ret = _server->AddService(forward_service,
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

  ForwardServer::Ptr build() {
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

    auto server = std::make_shared<ForwardServer>(_server);
    return server;
  }

 private:
  ServiceRegistry::Ptr _registry_client;
  ServiceDiscovery::Ptr _discovery_client;
  std::string _exchange_name;
  MQClient::Ptr _mq_client;
  std::shared_ptr<odb::core::database> _mysql_client;
  std::shared_ptr<brpc::Server> _server;

  std::string _user_service_name;
  ServiceManager::Ptr _service_manager;
};

}  // namespace huzch