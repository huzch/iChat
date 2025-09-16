#pragma once
#include <odb/core.hxx>
#include <odb/nullable.hxx>
#include <string>

namespace huzch {

#pragma db object table("session_member")
class SessionMember {
 public:
  SessionMember(const std::string& session_id, const std::string& user_id)
      : _session_id(session_id), _user_id(user_id) {}

  void session_id(const std::string& val) { _session_id = val; }
  std::string session_id() { return _session_id; }

  void user_id(const std::string& val) { _user_id = val; }
  std::string user_id() { return _user_id; }

 private:
  friend class odb::access;
#pragma db id auto
  unsigned long _id;
#pragma db type("varchar(64)")
  std::string _session_id;
#pragma db type("varchar(64)")
  std::string _user_id;
};

}  // namespace huzch