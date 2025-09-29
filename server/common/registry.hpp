#pragma once
#include <etcd/Client.hpp>
#include <etcd/KeepAlive.hpp>
#include <etcd/Response.hpp>
#include <etcd/Watcher.hpp>

#include "logger.hpp"

namespace huzch {

// 服务注册
class ServiceRegistry {
 public:
  using Ptr = std::shared_ptr<ServiceRegistry>;

 public:
  ServiceRegistry(const std::string& host, int ttl = 3)
      : _client(std::make_shared<etcd::Client>(host)),
        _keep_alive(_client->leasekeepalive(ttl).get()),
        _lease_id(_keep_alive->Lease()) {}

  ~ServiceRegistry() { _keep_alive->Cancel(); }

  bool register_service(const std::string& key, const std::string& val) {
    auto resp = _client->put(key, val, _lease_id).get();
    if (!resp.is_ok()) {
      LOG_ERROR("服务注册失败: {}", resp.error_message());
      return false;
    }
    return true;
  }

 private:
  std::shared_ptr<etcd::Client> _client;
  std::shared_ptr<etcd::KeepAlive> _keep_alive;
  int64_t _lease_id;
};

// 服务发现
class ServiceDiscovery {
 public:
  using Ptr = std::shared_ptr<ServiceDiscovery>;
  using CallBack = std::function<void(std::string, std::string)>;

 public:
  ServiceDiscovery(const std::string& host, const std::string& base_dir,
                   const CallBack& put, const CallBack& del)
      : _client(std::make_shared<etcd::Client>(host)), _put(put), _del(del) {
    auto resp = _client->ls(base_dir).get();
    if (!resp.is_ok()) {
      LOG_ERROR("服务发现失败: {}", resp.error_message());
    }
    // 同步上线服务
    for (auto& value : resp.values()) {
      _put(value.key(), value.as_string());
    }
    // 异步监控服务
    _watcher = std::make_shared<etcd::Watcher>(
        *_client, base_dir,
        std::bind(&ServiceDiscovery::call_back, this, std::placeholders::_1),
        true);
  }

  ~ServiceDiscovery() { _watcher->Cancel(); }

 private:
  void call_back(const etcd::Response& resp) {
    if (!resp.is_ok()) {
      LOG_ERROR("服务发现失败: {}", resp.error_message());
      return;
    }

    for (auto& event : resp.events()) {
      if (event.event_type() == etcd::Event::EventType::PUT) {
        _put(event.kv().key(), event.kv().as_string());
        LOG_DEBUG("{}-{} 服务上线", event.kv().key(), event.kv().as_string());
      } else if (event.event_type() == etcd::Event::EventType::DELETE_) {
        _del(event.prev_kv().key(), event.prev_kv().as_string());
        LOG_DEBUG("{}-{} 服务下线", event.prev_kv().key(),
                  event.prev_kv().as_string());
      }
    }
  }

 private:
  std::shared_ptr<etcd::Client> _client;
  std::shared_ptr<etcd::Watcher> _watcher;
  CallBack _put;
  CallBack _del;
};

}  // namespace huzch
