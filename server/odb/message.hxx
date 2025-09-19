#pragma once
#include <boost/date_time/posix_time/posix_time.hpp>
#include <odb/core.hxx>
#include <odb/nullable.hxx>
#include <string>

namespace huzch {

#pragma db object table("message")
class Message {
 public:
  Message() {}

  Message(const std::string& message_id, const std::string& session_id,
          const std::string& user_id, const unsigned char message_type,
          const boost::posix_time::ptime& create_time)
      : _message_id(message_id),
        _session_id(session_id),
        _user_id(user_id),
        _message_type(message_type),
        _create_time(create_time) {}

  void message_id(const std::string& val) { _message_id = val; }
  std::string message_id() { return _message_id; }

  void session_id(const std::string& val) { _session_id = val; }
  std::string session_id() { return _session_id; }

  void user_id(const std::string& val) { _user_id = val; }
  std::string user_id() { return _user_id; }

  void message_type(const unsigned char val) { _message_type = val; }
  unsigned char message_type() { return _message_type; }

  void create_time(const boost::posix_time::ptime& val) { _create_time = val; }
  boost::posix_time::ptime create_time() { return _create_time; }

  void content(const std::string& val) { _content = val; }
  std::string content() {
    if (_content.null()) {
      return std::string();
    }
    return *_content;
  }

  void file_id(const std::string& val) { _file_id = val; }
  std::string file_id() {
    if (_file_id.null()) {
      return std::string();
    }
    return *_file_id;
  }

  void file_name(const std::string& val) { _file_name = val; }
  std::string file_name() {
    if (_file_name.null()) {
      return std::string();
    }
    return *_file_name;
  }

  void file_size(const unsigned int val) { _file_size = val; }
  unsigned int file_size() {
    if (_file_size.null()) {
      return 0;
    }
    return *_file_size;
  }

 private:
  friend class odb::access;
#pragma db id auto
  unsigned long _id;
#pragma db type("varchar(64)") index unique
  std::string _message_id;
#pragma db type("varchar(64)") index
  std::string _session_id;
#pragma db type("varchar(64)")
  std::string _user_id;
  unsigned char _message_type;  // 0:string, 1:image, 2:speech, 3:file
#pragma db type("timestamp")
  boost::posix_time::ptime _create_time;
  odb::nullable<std::string> _content;  // string
#pragma db type("varchar(64)")
  odb::nullable<std::string> _file_id;  // not string
#pragma db type("varchar(64)")
  odb::nullable<std::string> _file_name;   // not string
  odb::nullable<unsigned int> _file_size;  // not string
};

}  // namespace huzch