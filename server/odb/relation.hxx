#pragma once
#include <odb/core.hxx>
#include <string>

namespace huzch {

// 成员关系
#pragma db object table("relation")
class Relation {
 public:
  Relation() {}

  Relation(const std::string& user_id, const std::string& peer_id)
      : _user_id(user_id), _peer_id(peer_id) {}

  void user_id(const std::string& val) { _user_id = val; }
  std::string user_id() const { return _user_id; }

  void peer_id(const std::string& val) { _peer_id = val; }
  std::string peer_id() const { return _peer_id; }

 private:
  friend class odb::access;
#pragma db id auto
  unsigned long _id;
#pragma db type("varchar(64)") index
  std::string _user_id;
#pragma db type("varchar(64)")
  std::string _peer_id;
};

}  // namespace huzch