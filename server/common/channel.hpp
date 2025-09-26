#pragma once
#include <brpc/channel.h>

#include <list>

#include "logger.hpp"

namespace huzch {

// 服务信道管理(管理一个服务对应的所有信道)
class ServiceChannel {
 public:
  using Ptr = std::shared_ptr<ServiceChannel>;
  using ChannelPtr = std::shared_ptr<brpc::Channel>;
  using ListIt = std::list<ChannelPtr>::iterator;

 public:
  ServiceChannel(const std::string& service_name)
      : _service_name(service_name) {}

  void insert(const std::string& host) {
    auto channel = std::make_shared<brpc::Channel>();

    brpc::ChannelOptions options;
    options.max_retry = -1;
    options.timeout_ms = -1;
    options.connect_timeout_ms = -1;
    options.protocol = brpc::PROTOCOL_BAIDU_STD;

    int ret = channel->Init(host.c_str(), &options);
    if (ret != 0) {
      LOG_ERROR("{}-{} 信道初始化失败", _service_name, host);
      return;
    }

    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _channels.insert(_channels.end(), channel);
    _host_it_map[host] = it;
  }

  void remove(const std::string& host) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_host_it_map.count(host)) {
      LOG_ERROR("{}-{} 信道不存在", _service_name, host);
      return;
    }

    auto it = _host_it_map[host];
    _channels.erase(it);
    _host_it_map.erase(host);
  }

  ChannelPtr get() {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_channels.size() == 0) {
      LOG_ERROR("当前没有 {} 服务节点", _service_name);
      return ChannelPtr();
    }

    if (_it == _channels.end()) {
      _it = _channels.begin();
    }

    auto ret = *_it;
    ++_it;
    return ret;
  }

 private:
  std::string _service_name;
  std::list<ChannelPtr> _channels;
  ListIt _it = _channels.begin();  // 轮转迭代器
  std::unordered_map<std::string, ListIt> _host_it_map;
  std::mutex _mutex;
};

// 服务管理(管理所有服务对应的所有信道)
class ChannelManager {
 public:
  using Ptr = std::shared_ptr<ChannelManager>;

 public:
  void declare(const std::string& service_name) {
    std::lock_guard<std::mutex> lock(_mutex);
    _services.insert(service_name);
  }

  void on_service_online(const std::string& service_instance,
                         const std::string& host) {
    auto service_name = get_service_name(service_instance);

    std::unique_lock<std::mutex> lock(_mutex);
    if (!_services.count(service_name)) {
      LOG_DEBUG("{} 服务上线节点: {}，但并不关心", service_name, host);
      return;
    }

    ServiceChannel::Ptr service;
    if (!_service_channel_map.count(service_name)) {
      service = std::make_shared<ServiceChannel>(service_name);
      _service_channel_map[service_name] = service;
    } else {
      service = _service_channel_map[service_name];
    }
    lock.unlock();

    service->insert(host);
    LOG_DEBUG("{} 服务上线节点: {}", service_name, host);
  }

  void on_service_offline(const std::string& service_instance,
                          const std::string& host) {
    auto service_name = get_service_name(service_instance);

    std::unique_lock<std::mutex> lock(_mutex);
    if (!_services.count(service_name)) {
      LOG_ERROR("{} 服务下线节点: {}，但并不关心", service_name, host);
      return;
    }

    ServiceChannel::Ptr service;
    if (!_service_channel_map.count(service_name)) {
      LOG_WARN("{} 服务下线节点: {} 时，未找到管理对象", service_name, host);
      return;
    } else {
      service = _service_channel_map[service_name];
    }
    lock.unlock();

    service->remove(host);
    LOG_DEBUG("{} 服务下线节点: {}", service_name, host);
  }

  ServiceChannel::ChannelPtr get(const std::string& service_name) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_service_channel_map.count(service_name)) {
      LOG_ERROR("当前没有 {} 服务节点", service_name);
      return ServiceChannel::ChannelPtr();
    }
    return _service_channel_map[service_name]->get();
  }

 private:
  std::string get_service_name(const std::string& service_instance) {
    size_t pos = service_instance.find_last_of('/');
    return service_instance.substr(0, pos);
  }

 private:
  std::unordered_set<std::string> _services;  // 关心的服务
  std::unordered_map<std::string, ServiceChannel::Ptr> _service_channel_map;
  std::mutex _mutex;
};

}  // namespace huzch