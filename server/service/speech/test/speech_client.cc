#include <gflags/gflags.h>

#include "asr.hpp"
#include "channel.hpp"
#include "registry.hpp"
#include "speech.pb.h"

DEFINE_bool(run_mode, true, "程序运行模式: true调试/false发布");
DEFINE_string(log_file, "", "发布模式下日志文件");
DEFINE_int32(log_level, 0, "发布模式下日志等级");

DEFINE_string(registry_host, "http://127.0.0.1:2379", "etcd服务器地址");
DEFINE_string(base_dir, "/service", "服务根目录");
DEFINE_string(speech_service_name, "/speech_service", "语音识别服务名");

int main(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  huzch::init_logger(FLAGS_run_mode, FLAGS_log_file, FLAGS_log_level);

  // 初始化rpc服务信道管理
  auto channels = std::make_shared<huzch::ChannelManager>();
  channels->declare(FLAGS_base_dir + FLAGS_speech_service_name);
  auto put_cb =
      std::bind(&huzch::ChannelManager::on_service_online, channels.get(),
                std::placeholders::_1, std::placeholders::_2);
  auto del_cb =
      std::bind(&huzch::ChannelManager::on_service_offline, channels.get(),
                std::placeholders::_1, std::placeholders::_2);

  // 初始化服务发现
  auto discovery_client = std::make_shared<huzch::ServiceDiscovery>(
      FLAGS_registry_host, FLAGS_base_dir, put_cb, del_cb);

  // 获取rpc服务信道
  auto channel = channels->get(FLAGS_base_dir + FLAGS_speech_service_name);
  if (!channel) {
    return -1;
  }

  // 读取语音文件
  std::string file_content;
  aip::get_file_content("16k.pcm", &file_content);

  // 发起rpc服务调用
  huzch::SpeechService_Stub stub(channel.get());
  brpc::Controller ctrl;
  huzch::SpeechRecognizeReq req;
  req.set_request_id("111");
  req.set_speech_content(file_content);
  huzch::SpeechRecognizeRsp rsp;

  stub.SpeechRecognize(&ctrl, &req, &rsp, nullptr);

  if (ctrl.Failed()) {
    huzch::LOG_ERROR("rpc调用失败: {}", ctrl.ErrorText());
    return -1;
  }
  if (!rsp.success()) {
    huzch::LOG_ERROR("rpc调用失败: {}", rsp.errmsg());
    return -1;
  }

  huzch::LOG_DEBUG("收到请求id: {}", rsp.request_id());
  huzch::LOG_DEBUG("收到识别结果: {}", rsp.recognition_result());

  return 0;
}