#pragma once
#include "data_odb.hpp"
#include "logger.hpp"
#include "relation-odb.hxx"
#include "relation.hxx"

namespace huzch {

class RelationTable {
 public:
  using Ptr = std::shared_ptr<RelationTable>;

 public:
  RelationTable(const std::shared_ptr<odb::core::database>& db)
      : _odb_client(db) {}

  bool insert(const std::string& user_id, const std::string& peer_id) {
    try {
      odb::transaction t(_odb_client->begin());
      Relation r1(user_id, peer_id);
      Relation r2(peer_id, user_id);
      _odb_client->persist(r1);
      _odb_client->persist(r2);
      t.commit();
    } catch (const std::exception& e) {
      LOG_ERROR("好友关系 {}-{} 新增失败: {}", user_id, peer_id, e.what());
      return false;
    }
    return true;
  }

  bool remove(const std::string& user_id, const std::string& peer_id) {
    try {
      odb::transaction t(_odb_client->begin());
      _odb_client->erase_query<Relation>(
          odb::query<Relation>::user_id == user_id &&
          odb::query<Relation>::peer_id == peer_id);
      _odb_client->erase_query<Relation>(
          odb::query<Relation>::user_id == peer_id &&
          odb::query<Relation>::peer_id == user_id);
      t.commit();
    } catch (const std::exception& e) {
      LOG_ERROR("好友关系 {}-{} 移除失败: {}", user_id, peer_id, e.what());
      return false;
    }
    return true;
  }

  bool exists(const std::string& user_id, const std::string& peer_id) {
    try {
      odb::transaction t(_odb_client->begin());
      auto result = _odb_client->query<Relation>(
          odb::query<Relation>::user_id == user_id &&
          odb::query<Relation>::peer_id == peer_id);
      bool ret = !result.empty();
      t.commit();
      return ret;
    } catch (const std::exception& e) {
      LOG_ERROR("好友关系 {}-{} 查询失败: {}", user_id, peer_id, e.what());
      return false;
    }
  }

  std::vector<std::string> friends_id(const std::string& user_id) {
    std::vector<std::string> friends_id;
    try {
      odb::transaction t(_odb_client->begin());
      auto result = _odb_client->query<Relation>(
          odb::query<Relation>::user_id == user_id);
      friends_id.reserve(result.size());
      for (const auto& relation : result) {
        friends_id.push_back(relation.peer_id());
      }
      t.commit();
    } catch (const std::exception& e) {
      LOG_ERROR("用户 {} 获取所有好友id失败: {}", user_id, e.what());
      return friends_id;
    }
    return friends_id;
  }

 private:
  std::shared_ptr<odb::core::database> _odb_client;
};

}  // namespace huzch