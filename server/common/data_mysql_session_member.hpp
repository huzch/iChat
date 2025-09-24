#pragma once
#include "data_mysql.hpp"
#include "logger.hpp"
#include "session_member-odb.hxx"
#include "session_member.hxx"

namespace huzch {

class SessionMemberTable {
 public:
  using Ptr = std::shared_ptr<SessionMemberTable>;

 public:
  SessionMemberTable(const std::shared_ptr<odb::core::database>& db)
      : _mysql_client(db) {}

  bool insert(SessionMember& member) {
    try {
      odb::transaction t(_mysql_client->begin());
      _mysql_client->persist(member);
      t.commit();
    } catch (const std::exception& e) {
      LOG_ERROR("会话 {} 新增单个成员 {} 失败: {}", member.session_id(),
                member.user_id(), e.what());
      return false;
    }
    return true;
  }

  bool insert(std::vector<SessionMember>& members) {
    try {
      odb::transaction t(_mysql_client->begin());
      for (auto& member : members) {
        _mysql_client->persist(member);
      }
      t.commit();
    } catch (const std::exception& e) {
      LOG_ERROR("会话 {} 新增 {} 个成员失败: {}", members[0].session_id(),
                members.size(), e.what());
      return false;
    }
    return true;
  }

  bool remove(const std::string& session_id, const std::string& user_id) {
    try {
      odb::transaction t(_mysql_client->begin());
      _mysql_client->erase_query<SessionMember>(
          odb::query<SessionMember>::session_id == session_id &&
          odb::query<SessionMember>::user_id == user_id);
      t.commit();
    } catch (const std::exception& e) {
      LOG_ERROR("会话 {} 移除单个成员 {} 失败: {}", session_id, user_id,
                e.what());
      return false;
    }
    return true;
  }

  bool remove(const std::string& session_id) {
    try {
      odb::transaction t(_mysql_client->begin());
      _mysql_client->erase_query<SessionMember>(
          odb::query<SessionMember>::session_id == session_id);
      t.commit();
    } catch (const std::exception& e) {
      LOG_ERROR("会话 {} 移除所有成员失败: {}", session_id, e.what());
      return false;
    }
    return true;
  }

  std::vector<SessionMember> members(const std::string& session_id) {
    std::vector<SessionMember> members;
    try {
      odb::transaction t(_mysql_client->begin());
      auto result = _mysql_client->query<SessionMember>(
          odb::query<SessionMember>::session_id == session_id);
      members.reserve(result.size());
      for (const auto& member : result) {
        members.push_back(member);
      }
      t.commit();
    } catch (const std::exception& e) {
      LOG_ERROR("会话 {} 获取所有成员失败: {}", session_id, e.what());
      return members;
    }
    return members;
  }

 private:
  std::shared_ptr<odb::core::database> _mysql_client;
};

}  // namespace huzch