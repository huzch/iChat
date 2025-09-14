#pragma once

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
                   .append("user_id", true, "keyword", "standard")
                   .append("avatar_id", false)
                   .append("name")
                   .append("phone", true, "keyword", "standard")
                   .append("description", false)
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
                           const std::vector<std::string>& user_id_list) {
    std::vector<User> users;
    Json::Value ret = ESSearch(_es_client, "user")
                          .append_must_not("user_id.keyword", user_id_list)
                          .append_should("user_id.keyword", key)
                          .append_should("name", key)
                          .append_should("phone.keyword", key)
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
      user.descrption(ret[i]["_source"]["descrption"].asString());
      users.push_back(user);
    }
    return users;
  }

 private:
  std::shared_ptr<elasticlient::Client> _es_client;
};

}  // namespace huzch