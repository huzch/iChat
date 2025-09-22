#pragma once
#include <odb/core.hxx>
#include <string>

#include "session_member.hxx"

namespace huzch {

// 聊天会话
#pragma db object table("session")
class Session {
 public:
  Session() {}

  Session(const std::string& session_id, const std::string& session_name)
      : _session_id(session_id), _session_name(session_name) {}

  void session_id(const std::string& val) { _session_id = val; }
  std::string session_id() { return _session_id; }

  void session_name(const std::string& val) { _session_name = val; }
  std::string session_name() { return _session_name; }

  void session_type(const unsigned char val) { _session_type = val; }
  unsigned char session_type() { return _session_type; }

 private:
  friend class odb::access;
#pragma db id auto
  unsigned long _id;
#pragma db type("varchar(64)") index unique
  std::string _session_id;
#pragma db type("varchar(64)")
  std::string _session_name;
#pragma db
  unsigned char _session_type;  // 0:single, 1:group
};

#pragma db view object(Session)                                              \
    object(SessionMember = m1 : Session::_session_id == m1::_session_id)     \
        object(SessionMember = m2 : Session::_session_id == m2::_session_id) \
            query((?))
struct SingleSession {
#pragma db column(Session::_session_id)
  std::string _session_id;
#pragma db column(m2::_user_id)
  std::string _friend_id;
};

#pragma db view object(Session)                                        \
    object(SessionMember = m : Session::_session_id == m::_session_id) \
        query((?))
struct GroupSession {
#pragma db column(Session::_session_id)
  std::string _session_id;
#pragma db column(Session::_session_name)
  std::string _session_name;
};

}  // namespace huzch