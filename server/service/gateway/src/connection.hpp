#pragma once
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "logger.hpp"

namespace huzch {

struct ClientInfo {
  ClientInfo(const std::string& user_id, const std::string& session_id)
      : _user_id(user_id), _session_id(session_id) {}

  std::string _user_id;
  std::string _session_id;
};

class Connection {
 public:
  using Ptr = std::shared_ptr<Connection>;
  using ConnectionPtr =
      websocketpp::server<websocketpp::config::asio>::connection_ptr;

 public:
  void insert(const ConnectionPtr& connection, const std::string& user_id,
              const std::string& session_id) {
    std::lock_guard<std::mutex> lock(_mutex);
    _user_connection_map[user_id] = connection;
    _connection_client_map[connection] = {user_id, session_id};
  }

  void remove(const ConnectionPtr& connection) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_connection_client_map.count(connection)) {
      LOG_ERROR("长连接 {} 不存在", (size_t)connection.get());
      return;
    }

    auto client_info = _connection_client_map[connection];
    _user_connection_map.erase(client_info._user_id);
    _connection_client_map.erase(connection);
  }

  bool client(const ConnectionPtr& connection, std::string& user_id,
              std::string& session_id) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_connection_client_map.count(connection)) {
      LOG_ERROR("长连接 {} 不存在", (size_t)connection.get());
      return false;
    }

    auto client_info = _connection_client_map[connection];
    user_id = client_info._user_id;
    session_id = client_info._session_id;
    return true;
  }

  ConnectionPtr get(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_user_connection_map.count(user_id)) {
      LOG_ERROR("用户 {} 不存在对应的长连接", user_id);
      return ConnectionPtr();
    }
    return _user_connection_map[user_id];
  }

 private:
  std::unordered_map<std::string, ConnectionPtr> _user_connection_map;
  std::unordered_map<ConnectionPtr, ClientInfo> _connection_client_map;
  std::mutex _mutex;
};

}  // namespace huzch