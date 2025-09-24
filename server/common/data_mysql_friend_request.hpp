#pragma once
#include "data_mysql.hpp"
#include "friend_request-odb.hxx"
#include "friend_request.hxx"
#include "logger.hpp"

namespace huzch {

class FriendRequestTable {
 public:
  using Ptr = std::shared_ptr<FriendRequestTable>;

 public:
  FriendRequestTable(const std::shared_ptr<odb::core::database>& db)
      : _mysql_client(db) {}

  bool insert(FriendRequest& friend_request) {
    try {
      odb::transaction t(_mysql_client->begin());
      _mysql_client->persist(friend_request);
      t.commit();
    } catch (const std::exception& e) {
      LOG_ERROR("好友申请 {}-{} 新增失败: {}", friend_request.user_id(),
                friend_request.peer_id(), e.what());
      return false;
    }
    return true;
  }

  bool remove(FriendRequest& friend_request) {
    try {
      odb::transaction t(_mysql_client->begin());
      _mysql_client->erase(friend_request);
      t.commit();
    } catch (const std::exception& e) {
      LOG_ERROR("好友申请 {}-{} 移除失败: {}", friend_request.user_id(),
                friend_request.peer_id(), e.what());
      return false;
    }
    return true;
  }

  bool exists(const std::string& user_id, const std::string& peer_id) {
    try {
      odb::transaction t(_mysql_client->begin());
      auto result = _mysql_client->query<FriendRequest>(
          odb::query<FriendRequest>::user_id == user_id &&
          odb::query<FriendRequest>::peer_id == peer_id);
      t.commit();
      return !result.empty();
    } catch (const std::exception& e) {
      LOG_ERROR("好友申请 {}-{} 查询失败: {}", user_id, peer_id, e.what());
      return false;
    }
  }

  std::vector<std::string> requesters(const std::string& user_id) {
    std::vector<std::string> requesters;
    try {
      odb::transaction t(_mysql_client->begin());
      auto result = _mysql_client->query<FriendRequest>(
          odb::query<FriendRequest>::peer_id == user_id);
      requesters.reserve(result.size());
      for (const auto& requester : result) {
        requesters.push_back(requester.user_id());
      }
      t.commit();
    } catch (const std::exception& e) {
      LOG_ERROR("用户 {} 获取所有好友申请用户id失败: {}", user_id, e.what());
      return requesters;
    }
    return requesters;
  }

 private:
  std::shared_ptr<odb::core::database> _mysql_client;
};

}  // namespace huzch