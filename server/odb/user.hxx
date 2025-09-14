#pragma once
#include <odb/core.hxx>
#include <odb/nullable.hxx>
#include <string>

namespace huzch {

#pragma db object table("user")
class User {
 public:
  User() {}

  User(const std::string &user_id, const std::string &name,
       const std::string &password)
      : _user_id(user_id), _name(name), _password(password) {}

  User(const std::string &user_id, const std::string &phone)
      : _user_id(user_id), _phone(phone) {}

  void user_id(const std::string &val) { _user_id = val; }
  std::string user_id() { return _user_id; }

  void avatar_id(const std::string &val) { _avatar_id = val; }
  std::string avatar_id() {
    if (_avatar_id.null()) {
      return std::string();
    }
    return *_avatar_id;
  }

  void name(const std::string &val) { _name = val; }
  std::string name() {
    if (_name.null()) {
      return std::string();
    }
    return *_name;
  }

  void phone(const std::string &val) { _phone = val; }
  std::string phone() {
    if (_phone.null()) {
      return std::string();
    }
    return *_phone;
  }

  void password(const std::string &val) { _password = val; }
  std::string password() {
    if (_password.null()) {
      return std::string();
    }
    return *_password;
  }

  void description(const std::string &val) { _description = val; }
  std::string description() {
    if (_description.null()) {
      return std::string();
    }
    return *_description;
  }

 private:
  friend class odb::access;
#pragma db id auto
  unsigned long _id;
#pragma db type("varchar(64)") index unique
  std::string _user_id;
#pragma db type("varchar(64)")
  odb::nullable<std::string> _avatar_id;
#pragma db type("varchar(64)") index unique
  odb::nullable<std::string> _name;
#pragma db type("varchar(64)") index unique
  odb::nullable<std::string> _phone;
#pragma db type("varchar(64)")
  odb::nullable<std::string> _password;
  odb::nullable<std::string> _description;
};

}  // namespace huzch