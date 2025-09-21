#pragma once
#include "data_mysql.hpp"
#include "logger.hpp"
#include "user-odb.hxx"
#include "user.hxx"

namespace huzch {

class UserTable {
 public:
  using Ptr = std::shared_ptr<UserTable>;

 public:
  UserTable(const std::shared_ptr<odb::core::database>& db)
      : _mysql_client(db) {}

  bool insert(const std::shared_ptr<User>& user) {
    try {
      odb::transaction t(_mysql_client->begin());
      _mysql_client->persist(*user);
      t.commit();
    } catch (const std::exception& e) {
      LOG_ERROR("用户 {} 新增失败: {}", user->name(), e.what());
      return false;
    }
    return true;
  }

  bool update(const std::shared_ptr<User>& user) {
    try {
      odb::transaction t(_mysql_client->begin());
      _mysql_client->update(*user);
      t.commit();
    } catch (const std::exception& e) {
      LOG_ERROR("用户 {} 更新失败: {}", user->name(), e.what());
      return false;
    }
    return true;
  }

  std::shared_ptr<User> select_by_name(const std::string& name) {
    try {
      odb::transaction t(_mysql_client->begin());
      auto user =
          _mysql_client->query_one<User>(odb::query<User>::name == name);
      t.commit();
      return std::shared_ptr<User>(user);
    } catch (const std::exception& e) {
      LOG_ERROR("昵称 {} 查询失败: {}", name, e.what());
      return nullptr;
    }
  }

  std::shared_ptr<User> select_by_phone(const std::string& phone) {
    try {
      odb::transaction t(_mysql_client->begin());
      auto user =
          _mysql_client->query_one<User>(odb::query<User>::phone == phone);
      t.commit();
      return std::shared_ptr<User>(user);
    } catch (const std::exception& e) {
      LOG_ERROR("手机号 {} 查询失败: {}", phone, e.what());
      return nullptr;
    }
  }

  std::shared_ptr<User> select_by_id(const std::string& user_id) {
    try {
      odb::transaction t(_mysql_client->begin());
      auto user =
          _mysql_client->query_one<User>(odb::query<User>::user_id == user_id);
      t.commit();
      return std::shared_ptr<User>(user);
    } catch (const std::exception& e) {
      LOG_ERROR("用户id {} 查询失败: {}", user_id, e.what());
      return nullptr;
    }
  }

  std::vector<User> select_by_multi_id(
      const std::vector<std::string>& users_id) {
    std::vector<User> users;
    try {
      odb::transaction t(_mysql_client->begin());

      std::string condition = "user_id in (";
      for (const auto& user_id : users_id) {
        condition += "'" + user_id + "',";
      }
      condition.pop_back();
      condition += ")";
      LOG_DEBUG("condition: {}", condition);

      auto result = _mysql_client->query<User>(condition);
      users.reserve(result.size());
      for (const auto& user : result) {
        users.push_back(user);
      }
      t.commit();
    } catch (const std::exception& e) {
      LOG_ERROR("用户id批量查询失败: {}", e.what());
    }
    return users;
  }

 private:
  std::shared_ptr<odb::core::database> _mysql_client;
};

}  // namespace huzch