#include "friend_server.hpp"

#include <gflags/gflags.h>

DEFINE_bool(run_mode, true, "程序运行模式: true调试/false发布");
DEFINE_string(log_file, "", "发布模式下日志文件");
DEFINE_int32(log_level, 0, "发布模式下日志等级");

DEFINE_string(registry_host, "http://127.0.0.1:2379", "etcd服务器地址");
DEFINE_string(base_dir, "/service", "服务根目录");
DEFINE_string(user_service_name, "/user_service", "用户服务名");
DEFINE_string(message_service_name, "/message_service", "消息服务名");
DEFINE_string(friend_service_name, "/friend_service", "好友服务名");
DEFINE_string(instance_name, "/instance", "实例名");
DEFINE_string(friend_service_host, "127.0.0.1:10006", "好友服务实例访问地址");

DEFINE_string(odb_host, "127.0.0.1", "odb服务器地址");
DEFINE_string(odb_user, "root", "odb服务器用户名");
DEFINE_string(odb_passwd, "123456", "odb服务器密码");
DEFINE_string(odb_db, "huzch", "odb默认库名");
DEFINE_string(odb_charset, "utf8", "odb客户端字符集");
DEFINE_int32(odb_port, 0, "odb服务器端口");
DEFINE_int32(odb_max_connections, 4, "odb连接池最大连接数量");

DEFINE_int32(rpc_port, 10006, "rpc服务器监听端口");
DEFINE_int32(rpc_timeout, -1, "rpc调用超时时间");
DEFINE_int32(rpc_threads, 1, "rpc的io线程数");

int main(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  huzch::init_logger(FLAGS_run_mode, FLAGS_log_file, FLAGS_log_level);

  huzch::FriendServerBuilder fsb;

  // 初始化服务注册
  fsb.init_registry_client(
      FLAGS_registry_host,
      FLAGS_base_dir + FLAGS_friend_service_name + FLAGS_instance_name,
      FLAGS_friend_service_host);

  // 初始化服务发现
  fsb.init_discovery_client(FLAGS_registry_host, FLAGS_base_dir,
                            FLAGS_user_service_name,
                            FLAGS_message_service_name);

  // 初始化odb数据库
  fsb.init_odb_client(FLAGS_odb_user, FLAGS_odb_passwd, FLAGS_odb_db,
                        FLAGS_odb_host, FLAGS_odb_port, FLAGS_odb_charset,
                        FLAGS_odb_max_connections);

  // 初始化rpc服务器
  fsb.init_rpc_server(FLAGS_rpc_port, FLAGS_rpc_timeout, FLAGS_rpc_threads);

  auto friend_server = fsb.build();
  friend_server->start();

  return 0;
}