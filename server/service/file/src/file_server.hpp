#pragma once
#include <brpc/server.h>

#include "base.pb.h"
#include "etcd.hpp"
#include "file.pb.h"
#include "utils.hpp"

namespace huzch {

class FileServiceImpl : public FileService {
 public:
  FileServiceImpl(const std::string& storage_path)
      : _storage_path(storage_path) {
    mode_t tmp = umask(0);
    mkdir(_storage_path.c_str(), 0775);
    umask(tmp);

    // 由于生成的uuid前没有/，所以字符串连接时加上
    if (_storage_path.back() != '/') {
      _storage_path.push_back('/');
    }
  }

  void GetSingleFile(google::protobuf::RpcController* controller,
                     const GetSingleFileReq* request,
                     GetSingleFileRsp* response,
                     google::protobuf::Closure* done) {
    brpc::ClosureGuard rpc_guard(done);
    std::string request_id = request->request_id();
    response->set_request_id(request_id);

    std::string body;
    std::string file_name = _storage_path + request->file_id();
    bool ret = read_file(file_name, body);
    if (!ret) {
      LOG_ERROR("{} 文件读取失败", request_id);
      response->set_success(false);
      response->set_errmsg("文件读取失败");
      return;
    }

    response->set_success(true);
    response->mutable_file_data()->set_file_id(request->file_id());
    response->mutable_file_data()->set_file_content(body);
  }

  void GetMultiFile(google::protobuf::RpcController* controller,
                    const GetMultiFileReq* request, GetMultiFileRsp* response,
                    google::protobuf::Closure* done) {
    brpc::ClosureGuard rpc_guard(done);
    std::string request_id = request->request_id();
    response->set_request_id(request_id);

    for (size_t i = 0; i < request->file_id_list_size(); ++i) {
      std::string body;
      std::string file_name = _storage_path + request->file_id_list(i);
      bool ret = read_file(file_name, body);
      if (!ret) {
        LOG_ERROR("{} 文件读取失败", request_id);
        response->set_success(false);
        response->set_errmsg("文件读取失败");
        return;
      }

      FileDownloadData data;
      data.set_file_id(request->file_id_list(i));
      data.set_file_content(body);
      response->mutable_file_data()->insert({request->file_id_list(i), data});
    }
    response->set_success(true);
  }

  void PutSingleFile(google::protobuf::RpcController* controller,
                     const PutSingleFileReq* request,
                     PutSingleFileRsp* response,
                     google::protobuf::Closure* done) {
    brpc::ClosureGuard rpc_guard(done);
    std::string request_id = request->request_id();
    response->set_request_id(request_id);

    std::string file_id = uuid();
    std::string file_name = _storage_path + file_id;
    bool ret = write_file(file_name, request->file_data().file_content());
    if (!ret) {
      LOG_ERROR("{} 文件写入失败", request_id);
      response->set_success(false);
      response->set_errmsg("文件写入失败");
      return;
    }

    response->set_success(true);
    response->mutable_file_info()->set_file_id(file_id);
    response->mutable_file_info()->set_file_size(
        request->file_data().file_size());
    response->mutable_file_info()->set_file_name(
        request->file_data().file_name());
  }

  void PutMultiFile(google::protobuf::RpcController* controller,
                    const PutMultiFileReq* request, PutMultiFileRsp* response,
                    google::protobuf::Closure* done) {
    brpc::ClosureGuard rpc_guard(done);
    std::string request_id = request->request_id();
    response->set_request_id(request_id);

    for (size_t i = 0; i < request->file_data_size(); ++i) {
      std::string file_id = uuid();
      std::string file_name = _storage_path + file_id;
      bool ret = write_file(file_name, request->file_data(i).file_content());
      if (!ret) {
        LOG_ERROR("{} 文件写入失败", request_id);
        response->set_success(false);
        response->set_errmsg("文件写入失败");
        return;
      }

      auto info = response->add_file_info();
      info->set_file_id(file_id);
      info->set_file_size(request->file_data(i).file_size());
      info->set_file_name(request->file_data(i).file_name());
    }
    response->set_success(true);
  }

 private:
  std::string _storage_path;
};

class FileServer {
 public:
  using Ptr = std::shared_ptr<FileServer>;

 public:
  FileServer(const std::shared_ptr<brpc::Server>& server) : _server(server) {}

  // 启动服务器
  void start() { _server->RunUntilAskedToQuit(); }

 private:
  std::shared_ptr<brpc::Server> _server;
};

class FileServerBuilder {
 public:
  void init_registry_client(const std::string& registry_host,
                            const std::string& service_name,
                            const std::string& service_host) {
    _registry_client = std::make_shared<ServiceRegistry>(registry_host);
    _registry_client->register_service(service_name, service_host);
  }

  void init_rpc_server(int port, int timeout, int num_threads,
                       const std::string& storage_path) {
    _server = std::make_shared<brpc::Server>();
    auto file_service = new FileServiceImpl(storage_path);
    int ret = _server->AddService(file_service,
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

  FileServer::Ptr build() {
    if (!_registry_client) {
      LOG_ERROR("未初始化服务注册模块");
      abort();
    }

    if (!_server) {
      LOG_ERROR("未初始化rpc服务器模块");
      abort();
    }

    auto server = std::make_shared<FileServer>(_server);
    return server;
  }

 private:
  ServiceRegistry::Ptr _registry_client;
  std::shared_ptr<brpc::Server> _server;
};

}  // namespace huzch
