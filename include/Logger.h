#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/async.h>
#include <memory>

class Logger
{
private:
    static inline std::unique_ptr<spdlog::logger> instance_{nullptr};

    Logger() = delete;

public:
    static void Init(const std::string &filename = "logs/system_log.txt");

    template <typename... Args>
    static inline void Info( const spdlog::source_loc &loc, spdlog::string_view_t msg, Args &&...args) noexcept
    {
        instance_->log(loc, spdlog::level::info, fmt::runtime(msg), std::forward<Args>(args)...);
    }

    template <typename... Args>
    static inline void Error(const spdlog::source_loc &loc, spdlog::string_view_t msg, Args &&...args) noexcept
    {
        instance_->log(loc, spdlog::level::err, fmt::runtime(msg), std::forward<Args>(args)...);
    }

    template <typename... Args>
    static inline void Warn(const spdlog::source_loc &loc, spdlog::string_view_t msg, Args &&...args) noexcept
    {
        instance_->log(loc, spdlog::level::warn, fmt::runtime(msg), std::forward<Args>(args)...);
    }

    template <typename... Args>
    static inline void Debug(const spdlog::source_loc &loc, spdlog::string_view_t msg, Args &&...args) noexcept
    {
        instance_->log(loc, spdlog::level::debug,  fmt::runtime(msg), std::forward<Args>(args)...);
    }
};

#define LOG_INFO(msg, ...) Logger::Info({__FILE__, __LINE__, __FUNCTION__}, msg, ##__VA_ARGS__)
#define LOG_ERROR(msg, ...) Logger::Error({__FILE__, __LINE__, __FUNCTION__}, msg, ##__VA_ARGS__)
#define LOG_WARN(msg, ...) Logger::Warn({__FILE__, __LINE__, __FUNCTION__}, msg, ##__VA_ARGS__)
#define LOG_DEBUG(msg, ...) Logger::Debug({__FILE__, __LINE__, __FUNCTION__}, msg, ##__VA_ARGS__)
