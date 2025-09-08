#pragma once
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>

#include "logger.hpp"

namespace huzch {

// 随机生成16位16进制字符
std::string uuid() {
  // 生成12位16进制字符 --- 6字节
  std::random_device rd;         // 设备随机数，真随机但效率低，作为种子
  std::mt19937 generator(rd());  // 生成伪随机数
  std::uniform_int_distribution<int> distribution(0, 255);  // 限定随机数范围

  std::stringstream ss;
  for (size_t i = 0; i < 6; ++i) {
    ss << std::setw(2) << std::setfill('0') << std::hex
       << distribution(generator);
  }

  // 生成4位16进制字符 --- 2字节
  static std::atomic<short> idx(0);
  short tmp = idx.fetch_add(1);
  ss << std::setw(4) << std::setfill('0') << std::hex << tmp;

  return ss.str();
}

bool read_file(const std::string& file_name, std::string& body) {
  std::ifstream ifs(file_name, std::ios::in | std::ios::binary | std::ios::ate);
  if (!ifs.is_open()) {
    LOG_ERROR("文件 {} 打开失败", file_name);
    return false;
  }

  size_t flen = ifs.tellg();
  ifs.seekg(0, std::ios::beg);
  body.resize(flen);

  ifs.read(&body[0], flen);
  if (!ifs.good()) {
    LOG_ERROR("文件 {} 读取失败", file_name);
    return false;
  }

  return true;
}

bool write_file(const std::string& file_name, const std::string& body) {
  std::ofstream ofs(file_name,
                    std::ios::out | std::ios::binary | std::ios::trunc);
  if (!ofs.is_open()) {
    LOG_ERROR("文件 {} 打开失败", file_name);
    return false;
  }

  ofs.write(body.c_str(), body.size());
  if (!ofs.good()) {
    LOG_ERROR("文件 {} 写入失败", file_name);
    return false;
  }

  return true;
}

}  // namespace huzch