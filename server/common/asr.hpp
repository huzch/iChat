#pragma once
#include "aip-cpp-sdk/speech.h"
#include "logger.hpp"

namespace huzch {

class ASRClient {
 public:
  using Ptr = std::shared_ptr<ASRClient>;

 public:
  ASRClient(const std::string& app_id, const std::string& api_key,
            const std::string& secret_key)
      : _client(app_id, api_key, secret_key) {}

  std::string recognize(const std::string& speech_data, std::string& err,
                        const std::string& format = "pcm") {
    Json::Value ret = _client.recognize(speech_data, format, 16000, aip::null);
    if (ret["err_no"].asInt() != 0) {
      LOG_ERROR("语音识别失败: {}", ret["err_msg"].asString());
      err = ret["err_msg"].asString();
      return "";
    }
    return ret["result"][0].asString();
  }

 private:
  aip::Speech _client;
};

}  // namespace huzch