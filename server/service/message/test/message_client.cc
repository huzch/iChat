#include <gflags/gflags.h>
#include <gtest/gtest.h>

#include "channel.hpp"
#include "etcd.hpp"
#include "message.pb.h"
#include "utils.hpp"

DEFINE_bool(run_mode, true, "程序运行模式: true调试/false发布");
DEFINE_string(log_file, "", "发布模式下日志文件");
DEFINE_int32(log_level, 0, "发布模式下日志等级");

DEFINE_string(registry_host, "http://127.0.0.1:2379", "etcd服务器地址");
DEFINE_string(base_dir, "/service", "服务根目录");
DEFINE_string(message_service_name, "/message_service", "转发服务名");

huzch::ServiceChannel::ChannelPtr channel;

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

  return RUN_ALL_TESTS();
}