#include "file_server.hpp"

#include <gflags/gflags.h>

DEFINE_bool(run_mode, true, "程序运行模式: true调试/false发布");
DEFINE_string(log_file, "", "发布模式下日志文件");
DEFINE_int32(log_level, 0, "发布模式下日志等级");

DEFINE_string(registry_host, "http://127.0.0.1:2379", "服务注册中心地址");
DEFINE_string(base_dir, "/service", "服务根目录");
DEFINE_string(service_name, "/file_service", "服务名");
DEFINE_string(instance_name, "/instance", "实例名");
DEFINE_string(access_host, "127.0.0.1:10002", "服务实例访问地址");

DEFINE_int32(rpc_port, 10002, "rpc服务器监听端口");
DEFINE_int32(rpc_timeout, -1, "rpc调用超时时间");
DEFINE_int32(rpc_threads, 1, "rpc的io线程数");

int main(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  huzch::init_logger(FLAGS_run_mode, FLAGS_log_file, FLAGS_log_level);

  huzch::FileServerBuilder fsb;

  // 初始化服务注册
  fsb.init_reg_client(FLAGS_registry_host,
                      FLAGS_base_dir + FLAGS_service_name + FLAGS_instance_name,
                      FLAGS_access_host);

  // 初始化rpc服务器
  fsb.init_rpc_server(FLAGS_rpc_port, FLAGS_rpc_timeout, FLAGS_rpc_threads);

  auto file_server = fsb.build();
  file_server->start();

  return 0;
}