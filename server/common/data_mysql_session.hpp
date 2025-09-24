#pragma once
#include "data_mysql.hpp"
#include "logger.hpp"
#include "session-odb.hxx"
#include "session.hxx"

namespace huzch {

class SessionTable {
 public:
  using Ptr = std::shared_ptr<SessionTable>;

 public:
  SessionTable(const std::shared_ptr<odb::core::database>& db)
      : _mysql_client(db) {}

  bool insert(Session& session) {
    try {
      odb::transaction t(_mysql_client->begin());
      _mysql_client->persist(session);
      t.commit();
    } catch (const std::exception& e) {
      LOG_ERROR("会话 {} 新增失败: {}", session.session_name(), e.what());
      return false;
    }
    return true;
  }

  bool remove(Session& session) {
    try {
      odb::transaction t(_mysql_client->begin());
      _mysql_client->erase(session);
      _mysql_client->erase_query<SessionMember>(
          odb::query<SessionMember>::session_id == session.session_id());
      t.commit();
    } catch (const std::exception& e) {
      LOG_ERROR("会话 {} 移除失败: {}", session.session_name(), e.what());
      return false;
    }
    return true;
  }

  std::shared_ptr<Session> select(const std::string& session_id) {
    try {
      odb::transaction t(_mysql_client->begin());
      auto session = _mysql_client->query_one<Session>(
          odb::query<Session>::session_id == session_id);
      t.commit();
      return std::shared_ptr<Session>(session);
    } catch (const std::exception& e) {
      LOG_ERROR("会话 {} 查询失败: {}", session_id, e.what());
      return nullptr;
    }
  }

  std::vector<SingleSession> single_sessions(const std::string& user_id) {
    std::vector<SingleSession> sessions;
    try {
      odb::transaction t(_mysql_client->begin());
      auto result = _mysql_client->query<SingleSession>(
          odb::query<SingleSession>::Session::session_type ==
              SessionType::SINGLE &&
          odb::query<SingleSession>::m1::user_id == user_id &&
          odb::query<SingleSession>::m2::user_id != user_id);
      sessions.reserve(result.size());
      for (const auto& session : result) {
        sessions.push_back(session);
      }
      t.commit();
    } catch (const std::exception& e) {
      LOG_ERROR("用户 {} 单聊会话查询失败: {}", user_id, e.what());
    }
    return sessions;
  }

  std::vector<GroupSession> group_sessions(const std::string& user_id) {
    std::vector<GroupSession> sessions;
    try {
      odb::transaction t(_mysql_client->begin());
      auto result = _mysql_client->query<GroupSession>(
          odb::query<GroupSession>::Session::session_type ==
              SessionType::GROUP &&
          odb::query<GroupSession>::m::user_id == user_id);
      sessions.reserve(result.size());
      for (const auto& session : result) {
        sessions.push_back(session);
      }
      t.commit();
    } catch (const std::exception& e) {
      LOG_ERROR("用户 {} 群聊会话查询失败: {}", user_id, e.what());
    }
    return sessions;
  }

 private:
  std::shared_ptr<odb::core::database> _mysql_client;
};

}  // namespace huzch