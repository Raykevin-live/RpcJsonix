#pragma once
#include <chrono>
#include <ctime>
#include <string_view>
#include <iostream>
#include <source_location>
#include <fmt/core.h>

// 日志级别使用强类型枚举替代宏定义，提供更好的类型安全
enum class LogLevel {
    DEBUG = 0,
    INFO  = 1,
    ERROR = 2
};

// 编译期可配置的默认日志级别
constexpr LogLevel DEFAULT_LOG_LEVEL = LogLevel::DEBUG;

namespace common {
    // 获取当前时间的格式化字符串
    inline std::string GetCurrentTimeString() {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        struct tm lt;
        localtime_r(&now_c, &lt); // 使用更安全的localtime_s替代localtime
        
        char time_buf[32];
        std::strftime(time_buf, sizeof(time_buf), "%m-%d %T", &lt);
        return time_buf;
    }

    // 日志级别转字符串
    constexpr std::string_view LogLevelToString(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO:  return "INFO";
            case LogLevel::ERROR: return "ERROR";
            default: return "UNKNOWN";
        }
    }
} // detail

// 核心日志函数，使用C++20格式化和编译期条件判断
template <LogLevel Level, typename... Args>
constexpr void Log(std::string_view format, const std::string& file,
                    int line, Args&&... args) {
    // 编译期判断是否需要输出日志，不需要则完全不生成代码
    if constexpr (Level >= DEFAULT_LOG_LEVEL) {
        // 时间字符串在运行时获取
        std::string time_str = common::GetCurrentTimeString();
        // const std::source_location loc = std::source_location::current();
        // 使用C++20的std::format进行类型安全的格式化
        std::string log_message = fmt::format(
            "[{}] [{}:{}] {}\n",
            time_str,
            file,
            line,
            fmt::vformat(format, fmt::make_format_args(args...))
        );
        
        // 根据日志级别选择输出流
        if constexpr (Level == LogLevel::ERROR) {
            std::cerr << log_message;
        } else {
            std::cout << log_message;
        }
    }
}

#define LOG_DEBUG(format, ...) Log<LogLevel::DEBUG>(format, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_INFO(format, ...) Log<LogLevel::INFO>(format, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) Log<LogLevel::ERROR>(format, __FILE__, __LINE__, ##__VA_ARGS__)