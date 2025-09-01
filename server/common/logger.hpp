#pragma once
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace huzch {

std::shared_ptr<spdlog::logger> g_default_logger;

void init_logger(bool mode, const std::string& file, int level) {
  if (mode) {  // 调试模式
    g_default_logger = spdlog::stdout_color_mt("default_logger");
    g_default_logger->set_level(spdlog::level::level_enum::trace);
    g_default_logger->flush_on(spdlog::level::level_enum::trace);
  } else {  // 发布模式
    g_default_logger = spdlog::basic_logger_mt("default_logger", file);
    g_default_logger->set_level((spdlog::level::level_enum)level);
    g_default_logger->flush_on((spdlog::level::level_enum)level);
  }
  // [日志器][时:分:秒][线程][日志等级]日志内容
  g_default_logger->set_pattern("[%n][%H:%M:%S][%t][%l]%v");
}

// 使用宏包装，在日志前添加代码文件名和行号，便于追踪错误
#define LOG_TRACE(format, ...)                                        \
  g_default_logger->trace(std::string("[{}:{}] ") + format, __FILE__, \
                          __LINE__, ##__VA_ARGS__)
#define LOG_DEBUG(format, ...)                                        \
  g_default_logger->debug(std::string("[{}:{}] ") + format, __FILE__, \
                          __LINE__, ##__VA_ARGS__)
#define LOG_INFO(format, ...)                                                  \
  g_default_logger->info(std::string("[{}:{}] ") + format, __FILE__, __LINE__, \
                         ##__VA_ARGS__)
#define LOG_WARN(format, ...)                                                  \
  g_default_logger->warn(std::string("[{}:{}] ") + format, __FILE__, __LINE__, \
                         ##__VA_ARGS__)
#define LOG_ERROR(format, ...)                                        \
  g_default_logger->error(std::string("[{}:{}] ") + format, __FILE__, \
                          __LINE__, ##__VA_ARGS__)
#define LOG_CRITICAL(format, ...)                                        \
  g_default_logger->critical(std::string("[{}:{}] ") + format, __FILE__, \
                             __LINE__, ##__VA_ARGS__)

}