#pragma once
#include <amqpcpp.h>
#include <amqpcpp/libev.h>
#include <ev.h>

#include <thread>

#include "logger.hpp"

namespace huzch {

class MQClient {
 public:
  using MessageCallBack = std::function<void(const char*, uint64_t)>;

 public:
  MQClient(const std::string& user, const std::string& passwd,
           const std::string& host) {
    _loop = EV_DEFAULT;
    _handler = std::make_unique<AMQP::LibEvHandler>(_loop);

    auto url = "amqp://" + user + ":" + passwd + "@" + host + "/";
    _connection = std::make_unique<AMQP::TcpConnection>(_handler.get(), url);
    _channel = std::make_unique<AMQP::TcpChannel>(_connection.get());

    _loop_thread = std::thread([this]() { ev_run(_loop); });
  }

  ~MQClient() {
    ev_async watcher;
    ev_async_init(&watcher, watcher_cb);
    ev_async_start(_loop, &watcher);
    ev_async_send(_loop, &watcher);
    _loop_thread.join();
  }

  void declare(const std::string& exchange, const std::string& queue,
               const std::string& routing_key = "routing_key",
               AMQP::ExchangeType exchange_type = AMQP::ExchangeType::direct) {
    _channel->declareExchange(exchange, exchange_type)
        .onError([](const std::string& err) {
          LOG_ERROR("交换机声明失败: {}", err);
          abort();
        })
        .onSuccess([exchange]() { LOG_DEBUG("交换机 {} 声明成功", exchange); });

    _channel->declareQueue(queue)
        .onError([](const std::string& err) {
          LOG_ERROR("队列声明失败: {}", err);
          abort();
        })
        .onSuccess([queue]() { LOG_DEBUG("队列 {} 声明成功", queue); });

    _channel->bindQueue(exchange, queue, routing_key)
        .onError([](const std::string& err) {
          LOG_ERROR("交换机-队列 绑定失败: {}", err);
          abort();
        })
        .onSuccess([exchange, queue]() {
          LOG_DEBUG("交换机 {} - 队列 {} 绑定成功", exchange, queue);
        });
  }

  bool publish(const std::string& exchange, const std::string& msg,
               const std::string& routing_key = "routing_key") {
    bool ret = _channel->publish(exchange, routing_key, msg);
    if (!ret) {
      LOG_ERROR("交换机 {} 消息发布失败", exchange);
      return false;
    }
    return true;
  }

  void consume(const std::string& queue, const MessageCallBack& cb,
               const const std::string& tag = "consume_tag") {
    _channel->consume(queue, tag)
        .onReceived(
            [this, cb](const AMQP::Message& msg, uint64_t deliveryTag, bool) {
              cb(msg.body(), msg.bodySize());
              _channel->ack(deliveryTag);
            })
        .onError([queue](const std::string& err) {
          LOG_ERROR("队列 {} 消息订阅失败: {}", queue, err);
          abort();
        });
  }

 private:
  static void watcher_cb(struct ev_loop* loop, ev_async*, int) {
    ev_break(loop, EVBREAK_ALL);
  }

 private:
  struct ev_loop* _loop;
  std::unique_ptr<AMQP::LibEvHandler> _handler;
  std::unique_ptr<AMQP::TcpConnection> _connection;
  std::unique_ptr<AMQP::TcpChannel> _channel;
  std::thread _loop_thread;
};

}