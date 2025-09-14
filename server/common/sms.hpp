#pragma once
#include <string>

namespace huzch {

class SMSClient {
 public:
  static bool send(const std::string& phone, const std::string& code) {}
};

}  // namespace huzch