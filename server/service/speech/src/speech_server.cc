#include "speech_server.hpp"

#include <gflags/gflags.h>

DEFINE_bool(run_mode, true, "程序运行模式: true调试/false发布");
DEFINE_string(log_file, "", "发布模式下日志文件");
DEFINE_int32(log_level, 0, "发布模式下日志等级");

DEFINE_string(asr_app_id, "119899216", "语音识别平台应用id");
DEFINE_string(asr_api_key, "pri3F79iHqXUU6JST535oOR7", "语音识别平台api密钥");
DEFINE_string(asr_secret_key, "2priN4V38gvixRX8R25N16q33uGoS8wg",
              "语音识别平台加密密钥");

DEFINE_string(registry_host, "http://127.0.0.1:2379", "etcd服务器地址");
DEFINE_string(base_dir, "/service", "服务根目录");
DEFINE_string(speech_service_name, "/speech_service", "语音识别服务名");
DEFINE_string(instance_name, "/instance", "实例名");
DEFINE_string(speech_service_host, "127.0.0.1:10001",
              "语音识别服务实例访问地址");

DEFINE_int32(rpc_port, 10001, "rpc服务器监听端口");
DEFINE_int32(rpc_timeout, -1, "rpc调用超时时间");
DEFINE_int32(rpc_threads, 1, "rpc的io线程数");

int main(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  huzch::init_logger(FLAGS_run_mode, FLAGS_log_file, FLAGS_log_level);

  huzch::SpeechServerBuilder ssb;

  // 初始化语音识别
  ssb.init_asr_client(FLAGS_asr_app_id, FLAGS_asr_api_key,
                      FLAGS_asr_secret_key);

  // 初始化服务注册
  ssb.init_registry_client(
      FLAGS_registry_host,
      FLAGS_base_dir + FLAGS_speech_service_name + FLAGS_instance_name,
      FLAGS_speech_service_host);

  // 初始化rpc服务器
  ssb.init_rpc_server(FLAGS_rpc_port, FLAGS_rpc_timeout, FLAGS_rpc_threads);

  auto speech_server = ssb.build();
  speech_server->start();

  return 0;
}