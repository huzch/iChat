#include "user_server.hpp"

#include <gflags/gflags.h>

DEFINE_bool(run_mode, true, "程序运行模式: true调试/false发布");
DEFINE_string(log_file, "", "发布模式下日志文件");
DEFINE_int32(log_level, 0, "发布模式下日志等级");

DEFINE_string(registry_host, "http://127.0.0.1:2379", "etcd服务器地址");
DEFINE_string(base_dir, "/service", "服务根目录");
DEFINE_string(user_service_name, "/user_service", "用户服务名");
DEFINE_string(file_service_name, "/file_service", "文件服务名");
DEFINE_string(instance_name, "/instance", "实例名");
DEFINE_string(user_service_host, "127.0.0.1:10003", "用户服务实例访问地址");

DEFINE_string(sms_key_id, "qwL1K8ekvzjW4nO0", "短信发送平台密钥id");

DEFINE_string(es_host, "http://127.0.0.1:9200/", "es搜索引擎服务器地址");

DEFINE_string(mysql_host, "127.0.0.1", "mysql服务器地址");
DEFINE_string(mysql_user, "root", "mysql服务器用户名");
DEFINE_string(mysql_passwd, "123456", "mysql服务器密码");
DEFINE_string(mysql_db, "huzch", "mysql默认库名");
DEFINE_string(mysql_charset, "utf8", "mysql客户端字符集");
DEFINE_int32(mysql_port, 0, "mysql服务器端口");
DEFINE_int32(mysql_max_connections, 4, "mysql连接池最大连接数量");

DEFINE_string(redis_host, "127.0.0.1", "redis服务器地址");
DEFINE_int32(redis_port, 6379, "redis服务器端口");
DEFINE_int32(redis_db, 0, "redis默认库号");
DEFINE_bool(redis_keep_alive, true, "redis长连接保活选项");

DEFINE_int32(rpc_port, 10003, "rpc服务器监听端口");
DEFINE_int32(rpc_timeout, -1, "rpc调用超时时间");
DEFINE_int32(rpc_threads, 1, "rpc的io线程数");

int main(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  huzch::init_logger(FLAGS_run_mode, FLAGS_log_file, FLAGS_log_level);

  huzch::UserServerBuilder usb;

  // 初始化服务注册
  usb.init_registry_client(
      FLAGS_registry_host,
      FLAGS_base_dir + FLAGS_user_service_name + FLAGS_instance_name,
      FLAGS_user_service_host);

  // 初始化服务发现
  usb.init_discovery_client(FLAGS_registry_host, FLAGS_base_dir,
                            FLAGS_file_service_name);

  // 初始化短信发送
  usb.init_sms_client(FLAGS_sms_key_id);

  // 初始化es搜索引擎
  usb.init_es_client({FLAGS_es_host});

  // 初始化mysql数据库
  usb.init_mysql_client(FLAGS_mysql_user, FLAGS_mysql_passwd, FLAGS_mysql_db,
                        FLAGS_mysql_host, FLAGS_mysql_port, FLAGS_mysql_charset,
                        FLAGS_mysql_max_connections);

  // 初始化redis数据库
  usb.init_redis_client(FLAGS_redis_host, FLAGS_redis_port, FLAGS_redis_db,
                        FLAGS_redis_keep_alive);

  // 初始化rpc服务器
  usb.init_rpc_server(FLAGS_rpc_port, FLAGS_rpc_timeout, FLAGS_rpc_threads);

  auto user_server = usb.build();
  user_server->start();

  return 0;
}