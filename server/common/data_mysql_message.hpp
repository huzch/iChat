#pragma once
#include "data_mysql.hpp"
#include "logger.hpp"
#include "message-odb.hxx"
#include "message.hxx"

namespace huzch {

class MessageTable {
 public:
  using Ptr = std::shared_ptr<MessageTable>;

 public:
  MessageTable(const std::shared_ptr<odb::core::database>& db)
      : _mysql_client(db) {}

  bool insert(Message& message) {
    try {
      odb::transaction t(_mysql_client->begin());
      _mysql_client->persist(message);
      t.commit();
    } catch (const std::exception& e) {
      LOG_ERROR("消息 {} 新增失败: {}", message.message_id(), e.what());
      return false;
    }
    return true;
  }

  bool remove(const std::string& session_id) {
    try {
      odb::transaction t(_mysql_client->begin());
      _mysql_client->erase_query<Message>(odb::query<Message>::session_id ==
                                          session_id);
      t.commit();
    } catch (const std::exception& e) {
      LOG_ERROR("会话 {} 移除所有消息失败: {}", session_id, e.what());
      return false;
    }
    return true;
  }

  std::vector<Message> recent(const std::string& session_id, size_t count) {
    std::vector<Message> messages;
    try {
      odb::transaction t(_mysql_client->begin());

      std::string condition = "session_id='" + session_id +
                              "' order by create_time desc limit " +
                              std::to_string(count);

      auto result = _mysql_client->query<Message>(condition);
      messages.reserve(result.size());
      for (const auto& message : result) {
        messages.push_back(message);
      }
      t.commit();
    } catch (const std::exception& e) {
      LOG_ERROR("会话 {} 获取最近 {} 条消息失败: {}", session_id, count,
                e.what());
      return messages;
    }
    return messages;
  }

  std::vector<Message> range(const std::string& session_id,
                             const boost::posix_time::ptime& start_time,
                             const boost::posix_time::ptime& end_time) {
    std::vector<Message> messages;
    try {
      odb::transaction t(_mysql_client->begin());
      auto result = _mysql_client->query<Message>(
          odb::query<Message>::session_id == session_id &&
          odb::query<Message>::create_time >= start_time &&
          odb::query<Message>::create_time <= end_time);
      messages.reserve(result.size());
      for (const auto& message : result) {
        messages.push_back(message);
      }
      t.commit();
    } catch (const std::exception& e) {
      LOG_ERROR("会话 {} 获取从 {} 到 {} 的消息失败: {}", session_id,
                boost::posix_time::to_simple_string(start_time),
                boost::posix_time::to_simple_string(end_time), e.what());
      return messages;
    }
    return messages;
  }

 private:
  std::shared_ptr<odb::core::database> _mysql_client;
};

}  // namespace huzch