#include <gflags/gflags.h>
#include <gtest/gtest.h>

#include "base.pb.h"
#include "channel.hpp"
#include "etcd.hpp"
#include "file.pb.h"
#include "utils.hpp"

DEFINE_bool(run_mode, true, "程序运行模式: true调试/false发布");
DEFINE_string(log_file, "", "发布模式下日志文件");
DEFINE_int32(log_level, 0, "发布模式下日志等级");

DEFINE_string(registry_host, "http://127.0.0.1:2379", "etcd服务器地址");
DEFINE_string(base_dir, "/service", "服务根目录");
DEFINE_string(file_service_name, "/file_service", "文件服务名");

huzch::ServiceChannel::ChannelPtr channel;
std::string single_file_id;
std::vector<std::string> multi_file_id;

TEST(put_test, single_test) {
  // 读取文件
  std::string body;
  ASSERT_TRUE(huzch::read_file("Makefile", body));

  // 上传文件
  huzch::FileService_Stub stub(channel.get());
  brpc::Controller ctrl;
  huzch::PutSingleFileReq req;
  req.set_request_id("111");
  req.mutable_file_data()->set_file_name("_Makefile");
  req.mutable_file_data()->set_file_size(body.size());
  req.mutable_file_data()->set_file_content(body);
  huzch::PutSingleFileRsp rsp;
  stub.PutSingleFile(&ctrl, &req, &rsp, nullptr);

  ASSERT_FALSE(ctrl.Failed());
  ASSERT_TRUE(rsp.success());
  single_file_id = rsp.file_info().file_id();
}

TEST(get_test, single_test) {
  // 下载文件
  huzch::FileService_Stub stub(channel.get());
  brpc::Controller ctrl;
  huzch::GetSingleFileReq req;
  req.set_request_id("222");
  req.set_file_id(single_file_id);
  huzch::GetSingleFileRsp rsp;
  stub.GetSingleFile(&ctrl, &req, &rsp, nullptr);

  ASSERT_FALSE(ctrl.Failed());
  ASSERT_TRUE(rsp.success());
  ASSERT_EQ(single_file_id, rsp.file_data().file_id());
  // 写入文件
  huzch::write_file("_Makefile", rsp.file_data().file_content());
}

TEST(put_test, multi_file) {
  // 读取文件
  std::string body1;
  ASSERT_TRUE(huzch::read_file("base.pb.h", body1));
  std::string body2;
  ASSERT_TRUE(huzch::read_file("file.pb.h", body2));

  // 上传文件
  huzch::FileService_Stub stub(channel.get());
  brpc::Controller ctrl;
  huzch::PutMultiFileReq req;
  req.set_request_id("333");

  auto file_data1 = req.add_files_data();
  file_data1->set_file_name("_base.pb.h");
  file_data1->set_file_size(body1.size());
  file_data1->set_file_content(body1);

  auto file_data2 = req.add_files_data();
  file_data2->set_file_name("_file.pb.h");
  file_data2->set_file_size(body2.size());
  file_data2->set_file_content(body2);

  huzch::PutMultiFileRsp rsp;
  stub.PutMultiFile(&ctrl, &req, &rsp, nullptr);

  ASSERT_FALSE(ctrl.Failed());
  ASSERT_TRUE(rsp.success());
  for (const auto& file_info : rsp.files_info()) {
    multi_file_id.push_back(file_info.file_id());
  }
}

TEST(get_test, multi_file) {
  // 下载文件
  huzch::FileService_Stub stub(channel.get());
  brpc::Controller ctrl;
  huzch::GetMultiFileReq req;
  req.set_request_id("444");
  for (const auto& file_id : multi_file_id) {
    req.add_files_id(file_id);
  }
  huzch::GetMultiFileRsp rsp;
  stub.GetMultiFile(&ctrl, &req, &rsp, nullptr);

  ASSERT_FALSE(ctrl.Failed());
  ASSERT_TRUE(rsp.success());
  for (const auto& file_id : multi_file_id) {
    ASSERT_TRUE(rsp.files_data().count(file_id));
  }
  // 写入文件
  auto map = rsp.files_data();
  huzch::write_file("_base.pb.h", map[multi_file_id[0]].file_content());
  huzch::write_file("_file.pb.h", map[multi_file_id[1]].file_content());
}

int main(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  testing::InitGoogleTest(&argc, argv);
  huzch::init_logger(FLAGS_run_mode, FLAGS_log_file, FLAGS_log_level);

  // 初始化rpc服务信道管理
  auto service_manager = std::make_shared<huzch::ServiceManager>();
  service_manager->declare(FLAGS_base_dir + FLAGS_file_service_name);
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
  channel = service_manager->get(FLAGS_base_dir + FLAGS_file_service_name);
  if (!channel) {
    return -1;
  }

  return RUN_ALL_TESTS();
}