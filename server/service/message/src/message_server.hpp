#pragma once
#include <brpc/server.h>

#include "base.pb.h"
#include "channel.hpp"
#include "data_mysql_message.hpp"
#include "data_search.hpp"
#include "registry.hpp"
#include "file.pb.h"
#include "message.pb.h"
#include "mq.hpp"
#include "user.pb.h"
#include "utils.hpp"

namespace huzch {

class MessageServiceImpl : public MessageService {
 public:
  MessageServiceImpl(const std::shared_ptr<elasticlient::Client>& es_client,
                     const std::shared_ptr<odb::core::database>& mysql_client,
                     const std::string& file_service_name,
                     const std::string& user_service_name,
                     const ChannelManager::Ptr& channels)
      : _es_message(std::make_shared<ESMessage>(es_client)),
        _mysql_message(std::make_shared<MessageTable>(mysql_client)),
        _file_service_name(file_service_name),
        _user_service_name(user_service_name),
        _channels(channels) {
    _es_message->index();
  }

  void GetHistoryMessage(google::protobuf::RpcController* controller,
                         const GetHistoryMessageReq* request,
                         GetHistoryMessageRsp* response,
                         google::protobuf::Closure* done) {
    brpc::ClosureGuard rpc_guard(done);
    std::string request_id = request->request_id();
    response->set_request_id(request_id);

    auto err_rsp = [response](const std::string& errmsg) {
      response->set_success(false);
      response->set_errmsg(errmsg);
    };
    std::string chat_session_id = request->chat_session_id();
    boost::posix_time::ptime start_time =
        boost::posix_time::from_time_t(request->start_time());
    boost::posix_time::ptime end_time =
        boost::posix_time::from_time_t(request->end_time());

    auto messages =
        _mysql_message->range(chat_session_id, start_time, end_time);

    std::unordered_set<std::string> users_id;
    for (auto& message : messages) {
      users_id.insert(message.user_id());
    }
    std::unordered_map<std::string, UserInfo> users_info;
    bool ret = get_user(request_id, users_id, users_info);
    if (!ret) {
      LOG_ERROR("{} 批量获取用户信息失败", request_id);
      err_rsp("批量获取用户信息失败");
      return;
    }

    std::unordered_set<std::string> files_id;
    for (auto& message : messages) {
      if (!message.file_id().empty()) {
        files_id.insert(message.file_id());
      }
    }
    std::unordered_map<std::string, std::string> files_data;
    ret = get_file(request_id, files_id, files_data);
    if (!ret) {
      LOG_ERROR("{} 批量下载文件失败", request_id);
      err_rsp("批量下载文件失败");
      return;
    }

    for (auto& message : messages) {
      auto message_info = response->add_messages_info();
      message_info->set_message_id(message.message_id());
      message_info->set_chat_session_id(message.session_id());
      message_info->set_timestamp(
          boost::posix_time::to_time_t(message.create_time()));
      message_info->mutable_sender()->CopyFrom(users_info[message.user_id()]);
      switch (message.message_type()) {
        case MessageType::STRING:
          message_info->mutable_message()->set_message_type(
              MessageType::STRING);
          message_info->mutable_message()
              ->mutable_string_message()
              ->set_content(message.content());
          break;
        case MessageType::SPEECH:
          message_info->mutable_message()->set_message_type(
              MessageType::SPEECH);
          message_info->mutable_message()
              ->mutable_speech_message()
              ->set_file_id(message.file_id());
          message_info->mutable_message()
              ->mutable_speech_message()
              ->set_file_content(files_data[message.file_id()]);
          break;
        case MessageType::IMAGE:
          message_info->mutable_message()->set_message_type(MessageType::IMAGE);
          message_info->mutable_message()->mutable_image_message()->set_file_id(
              message.file_id());
          message_info->mutable_message()
              ->mutable_image_message()
              ->set_file_content(files_data[message.file_id()]);
          break;
        case MessageType::FILE:
          message_info->mutable_message()->set_message_type(MessageType::FILE);
          message_info->mutable_message()->mutable_file_message()->set_file_id(
              message.file_id());
          message_info->mutable_message()
              ->mutable_file_message()
              ->set_file_size(message.file_size());
          message_info->mutable_message()
              ->mutable_file_message()
              ->set_file_name(message.file_name());
          message_info->mutable_message()
              ->mutable_file_message()
              ->set_file_content(files_data[message.file_id()]);
          break;
        default:
          LOG_ERROR("消息类型不合法");
          return;
      }
    }
    response->set_success(true);
  }

  void GetRecentMessage(google::protobuf::RpcController* controller,
                        const GetRecentMessageReq* request,
                        GetRecentMessageRsp* response,
                        google::protobuf::Closure* done) {
    brpc::ClosureGuard rpc_guard(done);
    std::string request_id = request->request_id();
    response->set_request_id(request_id);

    auto err_rsp = [response](const std::string& errmsg) {
      response->set_success(false);
      response->set_errmsg(errmsg);
    };
    std::string chat_session_id = request->chat_session_id();
    int msg_count = request->msg_count();

    auto messages = _mysql_message->recent(chat_session_id, msg_count);

    std::unordered_set<std::string> users_id;
    for (auto& message : messages) {
      users_id.insert(message.user_id());
    }
    std::unordered_map<std::string, UserInfo> users_info;
    bool ret = get_user(request_id, users_id, users_info);
    if (!ret) {
      LOG_ERROR("{} 批量获取用户信息失败", request_id);
      err_rsp("批量获取用户信息失败");
      return;
    }

    std::unordered_set<std::string> files_id;
    for (auto& message : messages) {
      if (!message.file_id().empty()) {
        files_id.insert(message.file_id());
      }
    }
    std::unordered_map<std::string, std::string> files_data;
    ret = get_file(request_id, files_id, files_data);
    if (!ret) {
      LOG_ERROR("{} 批量下载文件失败", request_id);
      err_rsp("批量下载文件失败");
      return;
    }

    for (auto& message : messages) {
      auto message_info = response->add_messages_info();
      message_info->set_message_id(message.message_id());
      message_info->set_chat_session_id(message.session_id());
      message_info->set_timestamp(
          boost::posix_time::to_time_t(message.create_time()));
      message_info->mutable_sender()->CopyFrom(users_info[message.user_id()]);
      switch (message.message_type()) {
        case MessageType::STRING:
          message_info->mutable_message()->set_message_type(
              MessageType::STRING);
          message_info->mutable_message()
              ->mutable_string_message()
              ->set_content(message.content());
          break;
        case MessageType::SPEECH:
          message_info->mutable_message()->set_message_type(
              MessageType::SPEECH);
          message_info->mutable_message()
              ->mutable_speech_message()
              ->set_file_id(message.file_id());
          message_info->mutable_message()
              ->mutable_speech_message()
              ->set_file_content(files_data[message.file_id()]);
          break;
        case MessageType::IMAGE:
          message_info->mutable_message()->set_message_type(MessageType::IMAGE);
          message_info->mutable_message()->mutable_image_message()->set_file_id(
              message.file_id());
          message_info->mutable_message()
              ->mutable_image_message()
              ->set_file_content(files_data[message.file_id()]);
          break;
        case MessageType::FILE:
          message_info->mutable_message()->set_message_type(MessageType::FILE);
          message_info->mutable_message()->mutable_file_message()->set_file_id(
              message.file_id());
          message_info->mutable_message()
              ->mutable_file_message()
              ->set_file_size(message.file_size());
          message_info->mutable_message()
              ->mutable_file_message()
              ->set_file_name(message.file_name());
          message_info->mutable_message()
              ->mutable_file_message()
              ->set_file_content(files_data[message.file_id()]);
          break;
        default:
          LOG_ERROR("消息类型不合法");
          return;
      }
    }
    response->set_success(true);
  }

  void MessageSearch(google::protobuf::RpcController* controller,
                     const MessageSearchReq* request,
                     MessageSearchRsp* response,
                     google::protobuf::Closure* done) {
    brpc::ClosureGuard rpc_guard(done);
    std::string request_id = request->request_id();
    response->set_request_id(request_id);

    auto err_rsp = [response](const std::string& errmsg) {
      response->set_success(false);
      response->set_errmsg(errmsg);
    };
    std::string chat_session_id = request->chat_session_id();
    std::string search_key = request->search_key();

    auto messages = _es_message->search(search_key, chat_session_id);

    std::unordered_set<std::string> users_id;
    for (auto& message : messages) {
      users_id.insert(message.user_id());
    }
    std::unordered_map<std::string, UserInfo> users_info;
    bool ret = get_user(request_id, users_id, users_info);
    if (!ret) {
      LOG_ERROR("{} 批量获取用户信息失败", request_id);
      err_rsp("批量获取用户信息失败");
      return;
    }

    for (auto& message : messages) {
      auto message_info = response->add_messages_info();
      message_info->set_message_id(message.message_id());
      message_info->set_chat_session_id(message.session_id());
      message_info->set_timestamp(
          boost::posix_time::to_time_t(message.create_time()));
      message_info->mutable_sender()->CopyFrom(users_info[message.user_id()]);
      message_info->mutable_message()->set_message_type(MessageType::STRING);
      message_info->mutable_message()->mutable_string_message()->set_content(
          message.content());
    }
    response->set_success(true);
  }

  void on_message(const char* body, uint64_t body_size) {
    MessageInfo message_info;
    bool ret = message_info.ParseFromArray(body, body_size);
    if (!ret) {
      LOG_ERROR("消息信息反序列化失败");
      return;
    }

    std::string content, file_id, file_name;
    uint64_t file_size;

    // 文本存储到es搜索引擎，非文本存储到文件
    switch (message_info.message().message_type()) {
      case MessageType::STRING:
        content = message_info.message().string_message().content();
        ret = _es_message->insert(
            message_info.message_id(), message_info.chat_session_id(),
            message_info.sender().user_id(), message_info.timestamp(), content);
        if (!ret) {
          LOG_ERROR("文本消息存储失败");
          return;
        }
        break;
      case MessageType::SPEECH:
        ret = put_file(
            file_id, "",
            message_info.message().speech_message().file_content().size(),
            message_info.message().speech_message().file_content());
        if (!ret) {
          LOG_ERROR("语音消息存储失败");
          return;
        }
        break;
      case MessageType::IMAGE:
        ret = put_file(
            file_id, "",
            message_info.message().image_message().file_content().size(),
            message_info.message().image_message().file_content());
        if (!ret) {
          LOG_ERROR("图片消息存储失败");
          return;
        }
        break;
      case MessageType::FILE:
        file_name = message_info.message().file_message().file_name();
        file_size = message_info.message().file_message().file_size();
        ret = put_file(file_id, file_name, file_size,
                       message_info.message().file_message().file_content());
        if (!ret) {
          LOG_ERROR("文件消息存储失败");
          return;
        }
        break;
      default:
        LOG_ERROR("消息类型不合法");
        return;
    }

    Message message(message_info.message_id(), message_info.chat_session_id(),
                    message_info.sender().user_id(),
                    message_info.message().message_type(),
                    boost::posix_time::from_time_t(message_info.timestamp()));
    message.content(content);
    message.file_id(file_id);
    message.file_name(file_name);
    message.file_size(file_size);
    ret = _mysql_message->insert(message);
    if (!ret) {
      LOG_ERROR("mysql新增消息失败");
      return;
    }

    LOG_DEBUG("消息存储完毕");
  }

 private:
  bool get_user(const std::string& request_id,
                const std::unordered_set<std::string> users_id,
                std::unordered_map<std::string, UserInfo>& users_info) {
    auto channel = _channels->get(_user_service_name);
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

  bool put_file(std::string& file_id, const std::string& file_name,
                const uint64_t file_size, const std::string& file_content) {
    std::string request_id = uuid();
    auto channel = _channels->get(_file_service_name);
    if (!channel) {
      LOG_ERROR("{} 未找到 {} 服务节点", request_id, _file_service_name);
      return false;
    }

    huzch::FileService_Stub stub(channel.get());
    brpc::Controller ctrl;
    huzch::PutSingleFileReq req;
    req.set_request_id(request_id);
    req.mutable_file_data()->set_file_name(file_name);
    req.mutable_file_data()->set_file_size(file_size);
    req.mutable_file_data()->set_file_content(file_content);
    huzch::PutSingleFileRsp rsp;

    stub.PutSingleFile(&ctrl, &req, &rsp, nullptr);
    if (ctrl.Failed() || !rsp.success()) {
      LOG_ERROR("{} {} 服务调用失败: {} {}", request_id, _file_service_name,
                ctrl.ErrorText(), rsp.errmsg());
      return false;
    }

    file_id = rsp.file_info().file_id();
    return true;
  }

 private:
  MessageTable::Ptr _mysql_message;
  ESMessage::Ptr _es_message;

  std::string _file_service_name;
  std::string _user_service_name;
  ChannelManager::Ptr _channels;
};

class MessageServer {
 public:
  using Ptr = std::shared_ptr<MessageServer>;

 public:
  MessageServer(const std::shared_ptr<brpc::Server>& server)
      : _server(server) {}

  // 启动服务器
  void start() { _server->RunUntilAskedToQuit(); }

 private:
  std::shared_ptr<brpc::Server> _server;
};

class MessageServerBuilder {
 public:
  void init_registry_client(const std::string& registry_host,
                            const std::string& service_name,
                            const std::string& service_host) {
    _registry_client = std::make_shared<ServiceRegistry>(registry_host);
    _registry_client->register_service(service_name, service_host);
  }

  void init_discovery_client(const std::string& registry_host,
                             const std::string& base_dir,
                             const std::string& file_service_name,
                             const std::string& user_service_name) {
    _file_service_name = base_dir + file_service_name;
    _user_service_name = base_dir + user_service_name;
    _channels = std::make_shared<ChannelManager>();
    _channels->declare(base_dir + file_service_name);
    _channels->declare(base_dir + user_service_name);
    auto put_cb = std::bind(&ChannelManager::on_service_online, _channels.get(),
                            std::placeholders::_1, std::placeholders::_2);
    auto del_cb =
        std::bind(&ChannelManager::on_service_offline, _channels.get(),
                  std::placeholders::_1, std::placeholders::_2);

    _discovery_client = std::make_shared<ServiceDiscovery>(
        registry_host, base_dir, put_cb, del_cb);
  }

  void init_mq_client(const std::string& user, const std::string& passwd,
                      const std::string& host, const std::string& exchange,
                      const std::string& queue, const std::string& routing_key) {
    _queue_name = queue;
    _mq_client = std::make_shared<MQClient>(user, passwd, host);
    _mq_client->declare(exchange, queue, routing_key);
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

  void init_rpc_server(int port, int timeout, int num_threads) {
    if (!_mq_client) {
      LOG_ERROR("未初始化rabbitmq消息队列模块");
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

    _server = std::make_shared<brpc::Server>();
    auto message_service =
        new MessageServiceImpl(_es_client, _mysql_client, _file_service_name,
                               _user_service_name, _channels);
    int ret = _server->AddService(message_service,
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

    auto msg_cb = std::bind(&MessageServiceImpl::on_message, message_service,
                            std::placeholders::_1, std::placeholders::_2);
    _mq_client->consume(_queue_name, msg_cb);
  }

  MessageServer::Ptr build() {
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

    return std::make_shared<MessageServer>(_server);
  }

 private:
  ServiceRegistry::Ptr _registry_client;
  ServiceDiscovery::Ptr _discovery_client;
  std::string _queue_name;
  MQClient::Ptr _mq_client;
  std::shared_ptr<elasticlient::Client> _es_client;
  std::shared_ptr<odb::core::database> _mysql_client;
  std::shared_ptr<brpc::Server> _server;

  std::string _file_service_name;
  std::string _user_service_name;
  ChannelManager::Ptr _channels;
};

}  // namespace huzch