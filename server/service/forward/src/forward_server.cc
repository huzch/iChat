#include "forward_server.hpp"

#include <gflags/gflags.h>

DEFINE_bool(run_mode, true, "程序运行模式: true调试/false发布");
DEFINE_string(log_file, "", "发布模式下日志文件");
DEFINE_int32(log_level, 0, "发布模式下日志等级");

DEFINE_string(registry_host, "http://127.0.0.1:2379", "etcd服务器地址");
DEFINE_string(base_dir, "/service", "服务根目录");
DEFINE_string(forward_service_name, "/forward_service", "转发服务名");
DEFINE_string(user_service_name, "/user_service", "用户服务名");
DEFINE_string(instance_name, "/instance", "实例名");
DEFINE_string(forward_service_host, "127.0.0.1:10004", "转发服务实例访问地址");

DEFINE_string(mq_host, "127.0.0.1:5672", "rabbitmq服务器地址");
DEFINE_string(mq_user, "root", "rabbitmq服务器用户名");
DEFINE_string(mq_passwd, "123456", "rabbitmq服务器密码");
DEFINE_string(mq_exchange, "msg_exchange", "持久化消息发布交换机名");
DEFINE_string(mq_queue, "msg_queue", "持久化消息发布队列名");
DEFINE_string(mq_routing_key, "msg_queue", "持久化消息发布路由键");

DEFINE_string(mysql_host, "127.0.0.1", "mysql服务器地址");
DEFINE_string(mysql_user, "root", "mysql服务器用户名");
DEFINE_string(mysql_passwd, "123456", "mysql服务器密码");
DEFINE_string(mysql_db, "huzch", "mysql默认库名");
DEFINE_string(mysql_charset, "utf8", "mysql客户端字符集");
DEFINE_int32(mysql_port, 0, "mysql服务器端口");
DEFINE_int32(mysql_max_connections, 4, "mysql连接池最大连接数量");

DEFINE_int32(rpc_port, 10004, "rpc服务器监听端口");
DEFINE_int32(rpc_timeout, -1, "rpc调用超时时间");
DEFINE_int32(rpc_threads, 1, "rpc的io线程数");

int main(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  huzch::init_logger(FLAGS_run_mode, FLAGS_log_file, FLAGS_log_level);

  huzch::ForwardServerBuilder fsb;

  // 初始化服务注册
  fsb.init_registry_client(
      FLAGS_registry_host,
      FLAGS_base_dir + FLAGS_forward_service_name + FLAGS_instance_name,
      FLAGS_forward_service_host);

  // 初始化服务发现
  fsb.init_discovery_client(FLAGS_registry_host, FLAGS_base_dir,
                            FLAGS_user_service_name);

  // 初始化rabbitmq消息队列
  fsb.init_mq_client(FLAGS_mq_user, FLAGS_mq_passwd, FLAGS_mq_host,
                     FLAGS_mq_exchange, FLAGS_mq_queue, FLAGS_mq_routing_key);

  // 初始化mysql数据库
  fsb.init_mysql_client(FLAGS_mysql_user, FLAGS_mysql_passwd, FLAGS_mysql_db,
                        FLAGS_mysql_host, FLAGS_mysql_port, FLAGS_mysql_charset,
                        FLAGS_mysql_max_connections);

  // 初始化rpc服务器
  fsb.init_rpc_server(FLAGS_rpc_port, FLAGS_rpc_timeout, FLAGS_rpc_threads);

  auto user_server = fsb.build();
  user_server->start();

  return 0;
}