#pragma once
#include <brpc/server.h>

#include "asr.hpp"
#include "registry.hpp"
#include "speech.pb.h"

namespace huzch {

class SpeechServiceImpl : public SpeechService {
 public:
  SpeechServiceImpl(const ASRClient::Ptr& asr_client)
      : _asr_client(asr_client) {}

  void SpeechRecognize(google::protobuf::RpcController* controller,
                       const SpeechRecognizeReq* request,
                       SpeechRecognizeRsp* response,
                       google::protobuf::Closure* done) {
    brpc::ClosureGuard rpc_guard(done);
    std::string request_id = request->request_id();
    response->set_request_id(request_id);

    std::string err;
    std::string ret = _asr_client->recognize(request->speech_content(), err);
    if (ret.empty()) {
      LOG_ERROR("{} 语音识别失败", request_id);
      response->set_success(false);
      response->set_errmsg("语音识别失败:" + err);
      return;
    }

    response->set_success(true);
    response->set_recognition_result(ret);
  }

 private:
  ASRClient::Ptr _asr_client;
};

class SpeechServer {
 public:
  using Ptr = std::shared_ptr<SpeechServer>;

 public:
  SpeechServer(const std::shared_ptr<brpc::Server>& server) : _server(server) {}

  // 启动服务器
  void start() { _server->RunUntilAskedToQuit(); }

 private:
  std::shared_ptr<brpc::Server> _server;
};

class SpeechServerBuilder {
 public:
  void init_asr_client(const std::string& app_id, const std::string& api_key,
                       const std::string& secret_key) {
    _asr_client = std::make_shared<ASRClient>(app_id, api_key, secret_key);
  }

  void init_registry_client(const std::string& registry_host,
                            const std::string& service_name,
                            const std::string& service_host) {
    _registry_client = std::make_shared<ServiceRegistry>(registry_host);
    _registry_client->register_service(service_name, service_host);
  }

  void init_rpc_server(int port, int timeout, int num_threads) {
    if (!_asr_client) {
      LOG_ERROR("未初始化语音识别模块");
      abort();
    }

    _server = std::make_shared<brpc::Server>();
    auto speech_service = new SpeechServiceImpl(_asr_client);
    int ret = _server->AddService(speech_service,
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

  SpeechServer::Ptr build() {
    if (!_registry_client) {
      LOG_ERROR("未初始化服务注册模块");
      abort();
    }

    if (!_server) {
      LOG_ERROR("未初始化rpc服务器模块");
      abort();
    }

    return std::make_shared<SpeechServer>(_server);
  }

 private:
  ASRClient::Ptr _asr_client;
  ServiceRegistry::Ptr _registry_client;
  std::shared_ptr<brpc::Server> _server;
};

}  // namespace huzch
