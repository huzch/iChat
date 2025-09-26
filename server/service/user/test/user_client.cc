#include <gflags/gflags.h>
#include <gtest/gtest.h>

#include "base.pb.h"
#include "channel.hpp"
#include "etcd.hpp"
#include "user.pb.h"
#include "utils.hpp"

DEFINE_bool(run_mode, true, "程序运行模式: true调试/false发布");
DEFINE_string(log_file, "", "发布模式下日志文件");
DEFINE_int32(log_level, 0, "发布模式下日志等级");

DEFINE_string(registry_host, "http://127.0.0.1:2379", "etcd服务器地址");
DEFINE_string(base_dir, "/service", "服务根目录");
DEFINE_string(user_service_name, "/user_service", "用户服务名");

huzch::ServiceChannel::ChannelPtr channel;
huzch::UserInfo user_info;

// TEST(register_test, name_register) {
//   huzch::UserService_Stub stub(channel.get());
//   brpc::Controller ctrl;
//   huzch::UserRegisterReq req;
//   req.set_request_id(huzch::uuid());
//   req.set_user_name(user_info.name());
//   req.set_password("abc123");
//   huzch::UserRegisterRsp rsp;

//   stub.UserRegister(&ctrl, &req, &rsp, nullptr);
//   ASSERT_FALSE(ctrl.Failed());
//   ASSERT_TRUE(rsp.success());
// }

// TEST(login_test, name_login) {
//   huzch::UserService_Stub stub(channel.get());
//   brpc::Controller ctrl;
//   huzch::UserLoginReq req;
//   req.set_request_id(huzch::uuid());
//   req.set_user_name(user_info.name());
//   req.set_password("abc123");
//   huzch::UserLoginRsp rsp;

//   stub.UserLogin(&ctrl, &req, &rsp, nullptr);
//   ASSERT_FALSE(ctrl.Failed());
//   ASSERT_TRUE(rsp.success());
// }

// TEST(set_test, set_avatar) {
//   huzch::UserService_Stub stub(channel.get());
//   brpc::Controller ctrl;
//   huzch::SetUserAvatarReq req;
//   req.set_request_id(huzch::uuid());
//   req.set_user_id(user_info.user_id());
//   req.set_avatar(user_info.avatar());
//   huzch::SetUserAvatarRsp rsp;

//   stub.SetUserAvatar(&ctrl, &req, &rsp, nullptr);
//   ASSERT_FALSE(ctrl.Failed());
//   ASSERT_TRUE(rsp.success());
// }

// TEST(set_test, set_description) {
//   huzch::UserService_Stub stub(channel.get());
//   brpc::Controller ctrl;
//   huzch::SetUserDescriptionReq req;
//   req.set_request_id(huzch::uuid());
//   req.set_user_id(user_info.user_id());
//   req.set_description(user_info.description());
//   huzch::SetUserDescriptionRsp rsp;

//   stub.SetUserDescription(&ctrl, &req, &rsp, nullptr);
//   ASSERT_FALSE(ctrl.Failed());
//   ASSERT_TRUE(rsp.success());
// }

// TEST(get_test, get_user_info) {
//   huzch::UserService_Stub stub(channel.get());
//   brpc::Controller ctrl;
//   huzch::GetUserInfoReq req;
//   req.set_request_id(huzch::uuid());
//   req.set_user_id(user_info.user_id());
//   huzch::GetUserInfoRsp rsp;

//   stub.GetUserInfo(&ctrl, &req, &rsp, nullptr);
//   ASSERT_FALSE(ctrl.Failed());
//   ASSERT_TRUE(rsp.success());
//   ASSERT_EQ(rsp.user_info().user_id(), user_info.user_id());
//   ASSERT_EQ(rsp.user_info().name(), user_info.name());
//   ASSERT_EQ(rsp.user_info().description(), user_info.description());
//   ASSERT_EQ(rsp.user_info().avatar(), user_info.avatar());
// }

// TEST(get_test, get_multi_user_info) {
//   huzch::UserService_Stub stub(channel.get());
//   brpc::Controller ctrl;
//   huzch::GetMultiUserInfoReq req;
//   req.set_request_id(huzch::uuid());
//   req.add_users_id("1fe9a0b8d11a0000");
//   req.add_users_id("cb40be03c9f00000");
//   huzch::GetMultiUserInfoRsp rsp;

//   stub.GetMultiUserInfo(&ctrl, &req, &rsp, nullptr);
//   ASSERT_FALSE(ctrl.Failed());
//   ASSERT_TRUE(rsp.success());
//   auto map = rsp.users_info();

//   auto user1 = map["1fe9a0b8d11a0000"];
//   ASSERT_EQ(user1.user_id(), "1fe9a0b8d11a0000");
//   ASSERT_EQ(user1.name(), "zhangsan");
//   ASSERT_EQ(user1.description(), "life is a game!");

//   auto user2 = map["cb40be03c9f00000"];
//   ASSERT_EQ(user2.user_id(), "cb40be03c9f00000");
//   ASSERT_EQ(user2.name(), "lisi");
//   ASSERT_EQ(user2.description(), "life is a game!");
//   // ASSERT_EQ(user2.avatar(), "这是一个帅气的头像");
// }

TEST(search_test, search_user_info) {
  huzch::UserService_Stub stub(channel.get());
  brpc::Controller ctrl;
  huzch::UserSearchReq req;
  req.set_request_id(huzch::uuid());
  req.set_search_key("lisi");
  req.set_user_id("d13c040f2fe60000");
  huzch::UserSearchRsp rsp;

  stub.UserSearch(&ctrl, &req, &rsp, nullptr);
  ASSERT_FALSE(ctrl.Failed());
  ASSERT_TRUE(rsp.success());

  for (auto& user_info : rsp.users_info()) {
    std::cout << user_info.user_id() << std::endl;
    std::cout << user_info.name() << std::endl;
    std::cout << user_info.phone() << std::endl;
    std::cout << user_info.description() << std::endl;
    std::cout << user_info.avatar() << std::endl;
  }
}

// TEST(set_test, set_name) {
//   huzch::UserService_Stub stub(channel.get());
//   brpc::Controller ctrl;
//   huzch::SetUserNameReq req;
//   req.set_request_id(huzch::uuid());
//   req.set_user_id(user_info.user_id());
//   req.set_user_name("zhangsan");
//   huzch::SetUserNameRsp rsp;

//   stub.SetUserName(&ctrl, &req, &rsp, nullptr);
//   ASSERT_FALSE(ctrl.Failed());
//   ASSERT_TRUE(rsp.success());
// }

std::string code_id;
std::string code;

void get_code() {
  huzch::UserService_Stub stub(channel.get());
  brpc::Controller ctrl;
  huzch::GetPhoneVerifyCodeReq req;
  req.set_request_id(huzch::uuid());
  req.set_phone_number(user_info.phone());
  huzch::GetPhoneVerifyCodeRsp rsp;

  stub.GetPhoneVerifyCode(&ctrl, &req, &rsp, nullptr);
  ASSERT_FALSE(ctrl.Failed());
  ASSERT_TRUE(rsp.success());

  code_id = rsp.verify_code_id();
}

// TEST(register_test, phone_register) {
//   get_code();

//   huzch::UserService_Stub stub(channel.get());
//   brpc::Controller ctrl;
//   huzch::PhoneRegisterReq req;
//   req.set_request_id(huzch::uuid());
//   req.set_phone_number(user_info.phone());
//   req.set_verify_code_id(code_id);
//   std::cout << "请输入验证码: ";
//   std::cin >> code;
//   req.set_verify_code(code);
//   huzch::PhoneRegisterRsp rsp;

//   stub.PhoneRegister(&ctrl, &req, &rsp, nullptr);
//   ASSERT_FALSE(ctrl.Failed());
//   ASSERT_TRUE(rsp.success());
// }

// TEST(login_test, phone_login) {
//   get_code();

//   huzch::UserService_Stub stub(channel.get());
//   brpc::Controller ctrl;
//   huzch::PhoneLoginReq req;
//   req.set_request_id(huzch::uuid());
//   req.set_phone_number(user_info.phone());
//   req.set_verify_code_id(code_id);
//   std::cout << "请输入验证码: ";
//   std::cin >> code;
//   req.set_verify_code(code);
//   huzch::PhoneLoginRsp rsp;

//   stub.PhoneLogin(&ctrl, &req, &rsp, nullptr);
//   ASSERT_FALSE(ctrl.Failed());
//   ASSERT_TRUE(rsp.success());
// }

// TEST(set_test, set_phone) {
//   get_code();

//   huzch::UserService_Stub stub(channel.get());
//   brpc::Controller ctrl;
//   huzch::SetUserPhoneNumberReq req;
//   req.set_request_id(huzch::uuid());
//   req.set_user_id(user_info.user_id());
//   req.set_phone_number("13825042245");
//   req.set_phone_verify_code_id(code_id);
//   std::cout << "请输入验证码: ";
//   std::cin >> code;
//   req.set_phone_verify_code(code);
//   huzch::SetUserPhoneNumberRsp rsp;

//   stub.SetUserPhoneNumber(&ctrl, &req, &rsp, nullptr);
//   ASSERT_FALSE(ctrl.Failed());
//   ASSERT_TRUE(rsp.success());
// }

int main(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  testing::InitGoogleTest(&argc, argv);
  huzch::init_logger(FLAGS_run_mode, FLAGS_log_file, FLAGS_log_level);

  // 初始化rpc服务信道管理
  auto channels = std::make_shared<huzch::ChannelManager>();
  channels->declare(FLAGS_base_dir + FLAGS_user_service_name);
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
  channel = channels->get(FLAGS_base_dir + FLAGS_user_service_name);
  if (!channel) {
    return -1;
  }

  user_info.set_user_id("cb40be03c9f00000");
  user_info.set_name("lisi");
  user_info.set_phone("13332844158");
  user_info.set_description("life is a game!");
  user_info.set_avatar("这是一个帅气的头像");

  return RUN_ALL_TESTS();
}