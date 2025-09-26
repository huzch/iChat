#include <gflags/gflags.h>
#include <gtest/gtest.h>

#include "channel.hpp"
#include "etcd.hpp"
#include "friend.pb.h"
#include "utils.hpp"

DEFINE_bool(run_mode, true, "程序运行模式: true调试/false发布");
DEFINE_string(log_file, "", "发布模式下日志文件");
DEFINE_int32(log_level, 0, "发布模式下日志等级");

DEFINE_string(registry_host, "http://127.0.0.1:2379", "etcd服务器地址");
DEFINE_string(base_dir, "/service", "服务根目录");
DEFINE_string(friend_service_name, "/friend_service", "好友服务名");

huzch::ServiceChannel::ChannelPtr channel;
std::string user_id;
std::string peer_id;
std::string chat_session_id;
bool agree;
std::string chat_session_name;
std::vector<std::string> users_id;

// TEST(request_test, send_request) {
//   huzch::FriendService_Stub stub(channel.get());
//   brpc::Controller ctrl;
//   huzch::FriendAddSendReq req;
//   req.set_request_id(huzch::uuid());
//   req.set_user_id(user_id);
//   req.set_respondent_id(peer_id);
//   huzch::FriendAddSendRsp rsp;

//   stub.FriendAddSend(&ctrl, &req, &rsp, nullptr);
//   ASSERT_FALSE(ctrl.Failed());
//   ASSERT_TRUE(rsp.success());
// }

// TEST(get_test, get_request) {
//   huzch::FriendService_Stub stub(channel.get());
//   brpc::Controller ctrl;
//   huzch::GetRequesterReq req;
//   req.set_request_id(huzch::uuid());
//   req.set_user_id(peer_id);
//   huzch::GetRequesterRsp rsp;

//   stub.GetRequester(&ctrl, &req, &rsp, nullptr);
//   ASSERT_FALSE(ctrl.Failed());
//   ASSERT_TRUE(rsp.success());

//   for (auto& requester_info : rsp.requesters_info()) {
//     std::cout << requester_info.user_id() << std::endl;
//     std::cout << requester_info.name() << std::endl;
//     std::cout << requester_info.phone() << std::endl;
//     std::cout << requester_info.description() << std::endl;
//     std::cout << requester_info.avatar() << std::endl;
//   }
// }

// TEST(request_test, process_request) {
//   huzch::FriendService_Stub stub(channel.get());
//   brpc::Controller ctrl;
//   huzch::FriendAddProcessReq req;
//   req.set_request_id(huzch::uuid());
//   req.set_agree(agree);
//   req.set_requester_id(user_id);
//   req.set_user_id(peer_id);
//   huzch::FriendAddProcessRsp rsp;

//   stub.FriendAddProcess(&ctrl, &req, &rsp, nullptr);
//   ASSERT_FALSE(ctrl.Failed());
//   ASSERT_TRUE(rsp.success());

//   if (agree) {
//     std::cout << rsp.chat_session_id() << std::endl;
//   }
// }

// TEST(get_test, get_friend) {
//   huzch::FriendService_Stub stub(channel.get());
//   brpc::Controller ctrl;
//   huzch::GetFriendReq req;
//   req.set_request_id(huzch::uuid());
//   req.set_user_id(user_id);
//   huzch::GetFriendRsp rsp;

//   stub.GetFriend(&ctrl, &req, &rsp, nullptr);
//   ASSERT_FALSE(ctrl.Failed());
//   ASSERT_TRUE(rsp.success());

//   for (auto& friend_info : rsp.friends_info()) {
//     std::cout << friend_info.user_id() << std::endl;
//     std::cout << friend_info.name() << std::endl;
//     std::cout << friend_info.phone() << std::endl;
//     std::cout << friend_info.description() << std::endl;
//     std::cout << friend_info.avatar() << std::endl;
//   }
// }

// TEST(remove_test, remove_friend) {
//   huzch::FriendService_Stub stub(channel.get());
//   brpc::Controller ctrl;
//   huzch::FriendRemoveReq req;
//   req.set_request_id(huzch::uuid());
//   req.set_user_id(user_id);
//   req.set_peer_id(peer_id);
//   req.set_chat_session_id(chat_session_id);
//   huzch::FriendRemoveRsp rsp;

//   stub.FriendRemove(&ctrl, &req, &rsp, nullptr);
//   ASSERT_FALSE(ctrl.Failed());
//   ASSERT_TRUE(rsp.success());
// }

// TEST(create_test, create_session) {
//   huzch::FriendService_Stub stub(channel.get());
//   brpc::Controller ctrl;
//   huzch::ChatSessionCreateReq req;
//   req.set_request_id(huzch::uuid());
//   req.set_chat_session_name(chat_session_name);
//   for (auto& user_id : users_id) {
//     req.add_members_id(user_id);
//   }
//   huzch::ChatSessionCreateRsp rsp;

//   stub.ChatSessionCreate(&ctrl, &req, &rsp, nullptr);
//   ASSERT_FALSE(ctrl.Failed());
//   ASSERT_TRUE(rsp.success());

//   std::cout << rsp.chat_session_info().chat_session_id() << std::endl;
//   std::cout << rsp.chat_session_info().chat_session_name() << std::endl;
// }

// TEST(get_test, get_session_member) {
//   huzch::FriendService_Stub stub(channel.get());
//   brpc::Controller ctrl;
//   huzch::GetChatSessionMemberReq req;
//   req.set_request_id(huzch::uuid());
//   req.set_chat_session_id(chat_session_id);
//   huzch::GetChatSessionMemberRsp rsp;

//   stub.GetChatSessionMember(&ctrl, &req, &rsp, nullptr);
//   ASSERT_FALSE(ctrl.Failed());
//   ASSERT_TRUE(rsp.success());

//   for (auto& member_info : rsp.members_info()) {
//     std::cout << member_info.user_id() << std::endl;
//     std::cout << member_info.name() << std::endl;
//     std::cout << member_info.phone() << std::endl;
//     std::cout << member_info.description() << std::endl;
//     std::cout << member_info.avatar() << std::endl;
//   }
// }

TEST(get_test, get_session) {
  huzch::FriendService_Stub stub(channel.get());
  brpc::Controller ctrl;
  huzch::GetChatSessionReq req;
  req.set_request_id(huzch::uuid());
  req.set_user_id(user_id);
  huzch::GetChatSessionRsp rsp;

  stub.GetChatSession(&ctrl, &req, &rsp, nullptr);
  ASSERT_FALSE(ctrl.Failed());
  ASSERT_TRUE(rsp.success());

  for (auto& chat_session_info : rsp.chat_sessions_info()) {
    std::cout << chat_session_info.single_chat_friend_id() << std::endl;
    std::cout << chat_session_info.chat_session_id() << std::endl;
    std::cout << chat_session_info.chat_session_name() << std::endl;
    std::cout << chat_session_info.avatar() << std::endl;

    auto message_info = chat_session_info.prev_message();
    std::cout << message_info.message_id() << std::endl;
    std::cout << message_info.chat_session_id() << std::endl;
    std::cout << message_info.timestamp() << std::endl;
    std::cout << message_info.sender().user_id() << std::endl;
    std::cout << message_info.sender().name() << std::endl;
    std::cout << message_info.sender().avatar() << std::endl;
    std::cout << message_info.message().message_type() << std::endl;
    std::cout << message_info.message().file_message().file_name() << std::endl;
    std::cout << message_info.message().file_message().file_content()
              << std::endl;
  }
}

int main(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  testing::InitGoogleTest(&argc, argv);
  huzch::init_logger(FLAGS_run_mode, FLAGS_log_file, FLAGS_log_level);

  // 初始化rpc服务信道管理
  auto channels = std::make_shared<huzch::ChannelManager>();
  channels->declare(FLAGS_base_dir + FLAGS_friend_service_name);
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
  channel = channels->get(FLAGS_base_dir + FLAGS_friend_service_name);
  if (!channel) {
    return -1;
  }

  user_id = "1fe9a0b8d11a0000";
  peer_id = "7b45ffc75e7b0001";
  chat_session_id = "7d84333082220002";
  agree = true;
  users_id = {"1fe9a0b8d11a0000", "cb40be03c9f00000", "7b45ffc75e7b0001"};

  return RUN_ALL_TESTS();
}