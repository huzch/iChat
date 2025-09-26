#include <gflags/gflags.h>
#include <gtest/gtest.h>

#include "channel.hpp"
#include "etcd.hpp"
#include "forward.pb.h"
#include "utils.hpp"

DEFINE_bool(run_mode, true, "程序运行模式: true调试/false发布");
DEFINE_string(log_file, "", "发布模式下日志文件");
DEFINE_int32(log_level, 0, "发布模式下日志等级");

DEFINE_string(registry_host, "http://127.0.0.1:2379", "etcd服务器地址");
DEFINE_string(base_dir, "/service", "服务根目录");
DEFINE_string(forward_service_name, "/forward_service", "转发服务名");

huzch::ServiceChannel::ChannelPtr channel;
std::string user_id;
std::string chat_session_id;
std::string content;
std::string file_name;

// TEST(forward_test, string_message) {
//   huzch::ForwardService_Stub stub(channel.get());
//   brpc::Controller ctrl;
//   huzch::NewMessageReq req;
//   req.set_request_id(huzch::uuid());
//   req.set_user_id(user_id);
//   req.set_chat_session_id(chat_session_id);
//   req.mutable_message()->set_message_type(huzch::MessageType::STRING);
//   req.mutable_message()->mutable_string_message()->set_content(content);
//   huzch::NewMessageRsp rsp;

//   stub.NewMessage(&ctrl, &req, &rsp, nullptr);
//   ASSERT_FALSE(ctrl.Failed());
//   ASSERT_TRUE(rsp.success());
// }

// TEST(forward_test, speech_message) {
//   huzch::ForwardService_Stub stub(channel.get());
//   brpc::Controller ctrl;
//   huzch::NewMessageReq req;
//   req.set_request_id(huzch::uuid());
//   req.set_user_id(user_id);
//   req.set_chat_session_id(chat_session_id);
//   req.mutable_message()->set_message_type(huzch::MessageType::SPEECH);
//   req.mutable_message()->mutable_speech_message()->set_file_content(content);
//   huzch::NewMessageRsp rsp;

//   stub.NewMessage(&ctrl, &req, &rsp, nullptr);
//   ASSERT_FALSE(ctrl.Failed());
//   ASSERT_TRUE(rsp.success());
// }

// TEST(forward_test, image_message) {
//   huzch::ForwardService_Stub stub(channel.get());
//   brpc::Controller ctrl;
//   huzch::NewMessageReq req;
//   req.set_request_id(huzch::uuid());
//   req.set_user_id(user_id);
//   req.set_chat_session_id(chat_session_id);
//   req.mutable_message()->set_message_type(huzch::MessageType::IMAGE);
//   req.mutable_message()->mutable_image_message()->set_file_content(content);
//   huzch::NewMessageRsp rsp;

//   stub.NewMessage(&ctrl, &req, &rsp, nullptr);
//   ASSERT_FALSE(ctrl.Failed());
//   ASSERT_TRUE(rsp.success());
// }

TEST(forward_test, file_message) {
  huzch::ForwardService_Stub stub(channel.get());
  brpc::Controller ctrl;
  huzch::NewMessageReq req;
  req.set_request_id(huzch::uuid());
  req.set_user_id(user_id);
  req.set_chat_session_id(chat_session_id);
  req.mutable_message()->set_message_type(huzch::MessageType::FILE);
  req.mutable_message()->mutable_file_message()->set_file_name(file_name);
  req.mutable_message()->mutable_file_message()->set_file_size(content.size());
  req.mutable_message()->mutable_file_message()->set_file_content(content);
  huzch::NewMessageRsp rsp;

  stub.NewMessage(&ctrl, &req, &rsp, nullptr);
  ASSERT_FALSE(ctrl.Failed());
  ASSERT_TRUE(rsp.success());
}

int main(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  testing::InitGoogleTest(&argc, argv);
  huzch::init_logger(FLAGS_run_mode, FLAGS_log_file, FLAGS_log_level);

  // 初始化rpc服务信道管理
  auto channels = std::make_shared<huzch::ChannelManager>();
  channels->declare(FLAGS_base_dir + FLAGS_forward_service_name);
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
  channel = channels->get(FLAGS_base_dir + FLAGS_forward_service_name);
  if (!channel) {
    return -1;
  }

  user_id = "cb40be03c9f00000";
  chat_session_id = "7d84333082220002";
  content = "哈哈哈哈哈哈";
  file_name = "搞笑文件";

  return RUN_ALL_TESTS();
}