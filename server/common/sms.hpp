#pragma once
#include <cpr/cpr.h>
#include <json/json.h>

#include "utils.hpp"

namespace huzch {

class SMSClient {
 public:
  using Ptr = std::shared_ptr<SMSClient>;

 public:
  SMSClient(const std::string& key_id) {
    _url = "https://push.spug.cc/send/" + key_id;
  }

  bool send(const std::string& phone, const std::string& code) {
    Json::Value data;
    data["name"] = "iChat";
    data["code"] = code;
    data["targets"] = phone;

    std::string body;
    serialize(data, body);
    // LOG_INFO("发送短信请求 - URL: {}", _url);
    // LOG_INFO("发送短信请求 - Body: {}", body);

    cpr::Response r =
        cpr::Post(cpr::Url{_url}, cpr::Body{body},
                  cpr::Header{{"Content-Type", "application/json"}},
                  cpr::VerifySsl{false});  // 跳过SSL验证

    // 检查 HTTP 状态码
    if (r.status_code != 200) {
      LOG_ERROR("短信验证码请求失败 - 状态码: {}, 响应: '{}', 错误: '{}'",
                r.status_code, r.text, r.error.message);
      return false;
    }

    LOG_INFO("短信发送成功");
    return true;
  }

 private:
  std::string _url;
};

}  // namespace huzch