#pragma once
#include <sw/redis++/redis.h>

namespace huzch {

class RedisClientFactory {
 public:
  static std::shared_ptr<sw::redis::Redis> create(const std::string& host,
                                                  int port, int db,
                                                  bool keep_alive) {
    sw::redis::ConnectionOptions options;
    options.host = host;
    options.port = port;
    options.db = db;
    options.keep_alive = keep_alive;
    return std::make_shared<sw::redis::Redis>(options);
  }
};

// 登录会话
class Session {
 public:
  using Ptr = std::shared_ptr<Session>;

 public:
  Session(const std::shared_ptr<sw::redis::Redis>& redis_client)
      : _redis_client(redis_client) {}

  bool insert(const std::string& session_id, const std::string& user_id) {
    return _redis_client->set(session_id, user_id);
  }

  bool remove(const std::string& session_id) {
    return _redis_client->del(session_id);
  }

  sw::redis::OptionalString user_id(const std::string& session_id) {
    return _redis_client->get(session_id);
  }

 private:
  std::shared_ptr<sw::redis::Redis> _redis_client;
};

// 登录状态
class Status {
 public:
  using Ptr = std::shared_ptr<Status>;

 public:
  Status(const std::shared_ptr<sw::redis::Redis>& redis_client)
      : _redis_client(redis_client) {}

  bool insert(const std::string& user_id) {
    return _redis_client->set(user_id, "");
  }

  bool remove(const std::string& user_id) {
    return _redis_client->del(user_id);
  }

  bool exists(const std::string& user_id) {
    return _redis_client->exists(user_id);
  }

 private:
  std::shared_ptr<sw::redis::Redis> _redis_client;
};

// 登录验证码
class Code {
 public:
  using Ptr = std::shared_ptr<Code>;

 public:
  Code(const std::shared_ptr<sw::redis::Redis>& redis_client)
      : _redis_client(redis_client) {}

  bool insert(
      const std::string& code_id, const std::string& code,
      const std::chrono::milliseconds& ttl = std::chrono::milliseconds(60000)) {
    return _redis_client->set(code_id, code, ttl);
  }

  bool remove(const std::string& code_id) {
    return _redis_client->del(code_id);
  }

  sw::redis::OptionalString code(const std::string& code_id) {
    return _redis_client->get(code_id);
  }

 private:
  std::shared_ptr<sw::redis::Redis> _redis_client;
};

}  // namespace huzch