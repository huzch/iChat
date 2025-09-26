#include <gflags/gflags.h>
#include <gtest/gtest.h>

#include <boost/date_time/posix_time/posix_time.hpp>

#include "channel.hpp"
#include "etcd.hpp"
#include "message.pb.h"
#include "utils.hpp"

DEFINE_bool(run_mode, true, "程序运行模式: true调试/false发布");
DEFINE_string(log_file, "", "发布模式下日志文件");
DEFINE_int32(log_level, 0, "发布模式下日志等级");

DEFINE_string(registry_host, "http://127.0.0.1:2379", "etcd服务器地址");
DEFINE_string(base_dir, "/service", "服务根目录");
DEFINE_string(message_service_name, "/message_service", "消息服务名");

huzch::ServiceChannel::ChannelPtr channel;
std::string chat_session_id;
int start_time;
int end_time;
int msg_count;
std::string search_key;

TEST(get_test, history_message) {
  huzch::MessageService_Stub stub(channel.get());
  brpc::Controller ctrl;
  huzch::GetHistoryMessageReq req;
  req.set_request_id(huzch::uuid());
  req.set_chat_session_id(chat_session_id);
  req.set_start_time(start_time);
  req.set_end_time(end_time);
  huzch::GetHistoryMessageRsp rsp;

  stub.GetHistoryMessage(&ctrl, &req, &rsp, nullptr);
  ASSERT_FALSE(ctrl.Failed());
  ASSERT_TRUE(rsp.success());

  for (const auto& message_info : rsp.messages_info()) {
    std::cout << message_info.message_id() << std::endl;
    std::cout << message_info.chat_session_id() << std::endl;
    std::cout << boost::posix_time::to_simple_string(
                     boost::posix_time::from_time_t(message_info.timestamp()))
              << std::endl;
    std::cout << message_info.sender().user_id() << std::endl;
    switch (message_info.message().message_type()) {
      case huzch::MessageType::STRING:
        std::cout << message_info.message().string_message().content()
                  << std::endl;
        break;
      case huzch::MessageType::SPEECH:
        std::cout << message_info.message().speech_message().file_content()
                  << std::endl;
        break;
      case huzch::MessageType::IMAGE:
        std::cout << message_info.message().image_message().file_content()
                  << std::endl;
        break;
      case huzch::MessageType::FILE:
        std::cout << message_info.message().file_message().file_name()
                  << std::endl;
        std::cout << message_info.message().file_message().file_content()
                  << std::endl;
        break;
      default:
        huzch::LOG_ERROR("消息类型不合法");
        break;
    }
  }
}

TEST(get_test, recent_message) {
  huzch::MessageService_Stub stub(channel.get());
  brpc::Controller ctrl;
  huzch::GetRecentMessageReq req;
  req.set_request_id(huzch::uuid());
  req.set_chat_session_id(chat_session_id);
  req.set_msg_count(msg_count);
  huzch::GetRecentMessageRsp rsp;

  stub.GetRecentMessage(&ctrl, &req, &rsp, nullptr);
  ASSERT_FALSE(ctrl.Failed());
  ASSERT_TRUE(rsp.success());

  for (const auto& message_info : rsp.messages_info()) {
    std::cout << message_info.message_id() << std::endl;
    std::cout << message_info.chat_session_id() << std::endl;
    std::cout << boost::posix_time::to_simple_string(
                     boost::posix_time::from_time_t(message_info.timestamp()))
              << std::endl;
    std::cout << message_info.sender().user_id() << std::endl;
    switch (message_info.message().message_type()) {
      case huzch::MessageType::STRING:
        std::cout << message_info.message().string_message().content()
                  << std::endl;
        break;
      case huzch::MessageType::SPEECH:
        std::cout << message_info.message().speech_message().file_content()
                  << std::endl;
        break;
      case huzch::MessageType::IMAGE:
        std::cout << message_info.message().image_message().file_content()
                  << std::endl;
        break;
      case huzch::MessageType::FILE:
        std::cout << message_info.message().file_message().file_name()
                  << std::endl;
        std::cout << message_info.message().file_message().file_content()
                  << std::endl;
        break;
      default:
        huzch::LOG_ERROR("消息类型不合法");
        break;
    }
  }
}

TEST(get_test, search_message) {
  huzch::MessageService_Stub stub(channel.get());
  brpc::Controller ctrl;
  huzch::MessageSearchReq req;
  req.set_request_id(huzch::uuid());
  req.set_chat_session_id(chat_session_id);
  req.set_search_key(search_key);
  huzch::MessageSearchRsp rsp;

  stub.MessageSearch(&ctrl, &req, &rsp, nullptr);
  ASSERT_FALSE(ctrl.Failed());
  ASSERT_TRUE(rsp.success());

  for (const auto& message_info : rsp.messages_info()) {
    std::cout << message_info.message_id() << std::endl;
    std::cout << message_info.chat_session_id() << std::endl;
    std::cout << boost::posix_time::to_simple_string(
                     boost::posix_time::from_time_t(message_info.timestamp()))
              << std::endl;
    std::cout << message_info.sender().user_id() << std::endl;
    std::cout << message_info.message().string_message().content() << std::endl;
  }
}

int main(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  testing::InitGoogleTest(&argc, argv);
  huzch::init_logger(FLAGS_run_mode, FLAGS_log_file, FLAGS_log_level);

  // 初始化rpc服务信道管理
  auto service_manager = std::make_shared<huzch::ServiceManager>();
  service_manager->declare(FLAGS_base_dir + FLAGS_message_service_name);
  auto put_cb = std::bind(&huzch::ServiceManager::on_service_online,
                          service_manager.get(), std::placeholders::_1,
                          std::placeholders::_2);
  auto del_cb = std::bind(&huzch::ServiceManager::on_service_offline,
                          service_manager.get(), std::placeholders::_1,
                          std::placeholders::_2);

  // 初始化服务发现
  auto discovery_client = std::make_shared<huzch::ServiceDiscovery>(
      FLAGS_registry_host, FLAGS_base_dir, put_cb, del_cb);

  // 获取rpc服务信道
  channel = service_manager->get(FLAGS_base_dir + FLAGS_message_service_name);
  if (!channel) {
    return -1;
  }

  chat_session_id = "s1";
  start_time = boost::posix_time::to_time_t(
      boost::posix_time::time_from_string("2025-09-22 06:11:01"));
  end_time = boost::posix_time::to_time_t(
      boost::posix_time::time_from_string("2025-09-22 06:25:16"));
  msg_count = 2;
  search_key = "你";

  return RUN_ALL_TESTS();
}