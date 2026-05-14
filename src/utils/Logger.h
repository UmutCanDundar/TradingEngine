// ======================================================================================================
// Logger
//
// PURPOSE:
//  - Centralized asynchronous logging mechanism for cold-path events, system errors,
//    and application diagnostics.
//  - Utilizes spdlog async logger with a thread-safe queue and background thread for
//    non-blocking logging. Supports log levels: info, warn, error, debug.
//
// THREAD SAFETY:
//  - Fully thread-safe; multiple threads can log concurrently without explicit locks.
//  - Asynchronous queue ensures logging does not block calling threads, making it safe
//    for cold-path error handling in ErrorHandler.
//
// PERFORMANCE & DESIGN NOTES:
//  - Designed for **cold-path usage**; hot-path performance-critical code (like market
//    data handlers or order routing) should not log frequently.
//  - Log messages are formatted with source location (`__FILE__`, `__LINE__`, `__FUNCTION__`)
//    for traceability.
//  - Flush policy: errors and above are flushed immediately; all other logs are flushed
//    every second to disk to balance reliability and performance.
//  - Singleton pattern via static inline `instance_` ensures one global logger instance.
//  - Idempotent Init ensures safe repeated calls without reinitialization.
//  - Template variadic functions handle formatting, no need for inline overhead in cold path.
//
// DEVELOPER NOTES:
//  - Logger is fully compatible with ErrorHandler for cold-path logging.
//========================================================================================================

#pragma once

#include <memory>
#include <utility>
#include <filesystem>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/async.h>

class Logger
{
private:
    static inline std::shared_ptr<spdlog::logger> instance_{nullptr};

    Logger() = delete;

public:
    static void Init(const std::filesystem::path& filename = std::filesystem::path(PROJECT_ROOT)/"logs"/"system_log.txt");
 
    static inline void Shutdown() { spdlog::shutdown(); }

    template <typename... Args>
    static void Info( const spdlog::source_loc &loc, spdlog::string_view_t msg, Args &&...args) 
    {
        instance_->log(loc, spdlog::level::info, fmt::runtime(msg), std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void Error(const spdlog::source_loc &loc, spdlog::string_view_t msg, Args &&...args)
    {
        instance_->log(loc, spdlog::level::err, fmt::runtime(msg), std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void Warn(const spdlog::source_loc &loc, spdlog::string_view_t msg, Args &&...args) 
    {
        instance_->log(loc, spdlog::level::warn, fmt::runtime(msg), std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void Debug(const spdlog::source_loc &loc, spdlog::string_view_t msg, Args &&...args) 
    {
        instance_->log(loc, spdlog::level::debug,  fmt::runtime(msg), std::forward<Args>(args)...);
    }
};

#define LOG_INFO(msg, ...) Logger::Info({__FILE__, __LINE__, __FUNCTION__}, msg __VA_OPT__(,) __VA_ARGS__)
#define LOG_ERROR(msg, ...) Logger::Error({__FILE__, __LINE__, __FUNCTION__}, msg __VA_OPT__(,) __VA_ARGS__)
#define LOG_WARN(msg, ...) Logger::Warn({__FILE__, __LINE__, __FUNCTION__}, msg __VA_OPT__(,) __VA_ARGS__)
#define LOG_DEBUG(msg, ...) Logger::Debug({__FILE__, __LINE__, __FUNCTION__}, msg __VA_OPT__(,) __VA_ARGS__)
