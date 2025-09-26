#include "gateway_server.hpp"

#include <gflags/gflags.h>

DEFINE_bool(run_mode, true, "程序运行模式: true调试/false发布");
DEFINE_string(log_file, "", "发布模式下日志文件");
DEFINE_int32(log_level, 0, "发布模式下日志等级");

DEFINE_string(registry_host, "http://127.0.0.1:2379", "etcd服务器地址");
DEFINE_string(base_dir, "/service", "服务根目录");
DEFINE_string(speech_service_name, "/speech_service", "语音识别服务名");
DEFINE_string(file_service_name, "/file_service", "文件服务名");
DEFINE_string(user_service_name, "/user_service", "用户服务名");
DEFINE_string(forward_service_name, "/forward_service", "转发服务名");
DEFINE_string(message_service_name, "/message_service", "消息服务名");
DEFINE_string(friend_service_name, "/friend_service", "好友服务名");

DEFINE_string(redis_host, "127.0.0.1", "redis服务器地址");
DEFINE_int32(redis_port, 6379, "redis服务器端口");
DEFINE_int32(redis_db, 0, "redis默认库号");
DEFINE_bool(redis_keep_alive, true, "redis长连接保活选项");

DEFINE_int32(http_port, 9000, "http服务器端口");
DEFINE_int32(websocket_port, 9001, "websocket服务器端口");

int main(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  huzch::init_logger(FLAGS_run_mode, FLAGS_log_file, FLAGS_log_level);

  huzch::GatewayServerBuilder gsb;

  // 初始化服务发现
  gsb.init_discovery_client(FLAGS_registry_host, FLAGS_base_dir,
                            FLAGS_speech_service_name, FLAGS_file_service_name,
                            FLAGS_user_service_name, FLAGS_forward_service_name,
                            FLAGS_message_service_name,
                            FLAGS_friend_service_name);

  // 初始化redis数据库
  gsb.init_redis_client(FLAGS_redis_host, FLAGS_redis_port, FLAGS_redis_db,
                        FLAGS_redis_keep_alive);

  // 初始化http-websocket服务器
  gsb.init_http_websocket_server(FLAGS_http_port, FLAGS_websocket_port);

  auto gateway_server = gsb.build();
  gateway_server->start();

  return 0;
}