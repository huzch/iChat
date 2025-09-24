#pragma once

#include "message.hxx"
#include "search.hpp"
#include "user.hxx"

namespace huzch {

class ESClientFactory {
 public:
  static std::shared_ptr<elasticlient::Client> create(
      const std::vector<std::string>& host_list) {
    return std::make_shared<elasticlient::Client>(host_list);
  }
};

class ESUser {
 public:
  using Ptr = std::shared_ptr<ESUser>;

 public:
  ESUser(const std::shared_ptr<elasticlient::Client>& es_client)
      : _es_client(es_client) {}

  bool index() {
    bool ret = ESIndex(_es_client, "user")
                   .append("user_id", "keyword", true, "standard")
                   .append("avatar_id", "keyword", false)
                   .append("name")
                   .append("phone", "keyword", true, "standard")
                   .append("description", "text", false)
                   .init();

    if (!ret) {
      LOG_ERROR("用户信息创建失败");
      return false;
    }
    return true;
  }

  bool insert(const std::string& user_id, const std::string& avatar_id,
              const std::string& name, const std::string& phone,
              const std::string& description) {
    bool ret = ESInsert(_es_client, "user")
                   .append("user_id", user_id)
                   .append("avatar_id", avatar_id)
                   .append("name", name)
                   .append("phone", phone)
                   .append("description", description)
                   .insert(user_id);

    if (!ret) {
      LOG_ERROR("用户信息插入失败");
      return false;
    }
    return true;
  }

  std::vector<User> search(const std::string& key,
                           const std::vector<std::string>& users_id) {
    std::vector<User> users;
    auto ret = ESSearch(_es_client, "user")
                   .append_must_not_terms("user_id.keyword", users_id)
                   .append_should_match("user_id.keyword", key)
                   .append_should_match("name", key)
                   .append_should_match("phone.keyword", key)
                   .search();

    if (!ret.isArray()) {
      LOG_ERROR("用户搜索结果不为数组类型");
      return users;
    }

    size_t sz = ret.size();
    users.reserve(sz);
    for (int i = 0; i < sz; ++i) {
      User user;
      user.user_id(ret[i]["_source"]["user_id"].asString());
      user.avatar_id(ret[i]["_source"]["avatar_id"].asString());
      user.name(ret[i]["_source"]["name"].asString());
      user.phone(ret[i]["_source"]["phone"].asString());
      user.description(ret[i]["_source"]["description"].asString());
      users.push_back(user);
    }
    return users;
  }

 private:
  std::shared_ptr<elasticlient::Client> _es_client;
};

class ESMessage {
 public:
  using Ptr = std::shared_ptr<ESMessage>;

 public:
  ESMessage(const std::shared_ptr<elasticlient::Client>& es_client)
      : _es_client(es_client) {}

  bool index() {
    bool ret = ESIndex(_es_client, "message")
                   .append("message_id", "keyword", false)
                   .append("session_id", "keyword", true, "standard")
                   .append("user_id", "keyword", false)
                   .append("create_time", "long", false)
                   .append("content")
                   .init();

    if (!ret) {
      LOG_ERROR("消息信息创建失败");
      return false;
    }
    return true;
  }

  bool insert(const std::string& message_id, const std::string& session_id,
              const std::string& user_id, const long create_time,
              const std::string& content) {
    bool ret = ESInsert(_es_client, "message")
                   .append("message_id", message_id)
                   .append("session_id", session_id)
                   .append("user_id", user_id)
                   .append("create_time", create_time)
                   .append("content", content)
                   .insert(message_id);

    if (!ret) {
      LOG_ERROR("消息信息插入失败");
      return false;
    }
    return true;
  }

  bool remove(const std::string& message_id) {
    bool ret = ESRemove(_es_client, "message").remove(message_id);

    if (!ret) {
      LOG_ERROR("消息信息移除失败");
      return false;
    }
    return true;
  }

  std::vector<Message> search(const std::string& key,
                              const std::string& session_id) {
    std::vector<Message> messages;
    auto ret = ESSearch(_es_client, "message")
                   .append_must_term("session_id.keyword", session_id)
                   .append_must_match("content", key)
                   .search();

    if (!ret.isArray()) {
      LOG_ERROR("用户搜索结果不为数组类型");
      return messages;
    }

    size_t sz = ret.size();
    messages.reserve(sz);
    for (int i = 0; i < sz; ++i) {
      Message message;
      message.message_id(ret[i]["_source"]["message_id"].asString());
      message.session_id(ret[i]["_source"]["session_id"].asString());
      message.user_id(ret[i]["_source"]["user_id"].asString());
      auto create_time = boost::posix_time::from_time_t(
          ret[i]["_source"]["create_time"].asInt64());
      message.create_time(create_time);
      message.content(ret[i]["_source"]["content"].asString());
      messages.push_back(message);
    }
    return messages;
  }

 private:
  std::shared_ptr<elasticlient::Client> _es_client;
};

}  // namespace huzch