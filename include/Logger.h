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
    static void Init(const std::string &filename = "logs/system_log.txt")
    {
        // Sadece ilk seferde kurulum yapılır (idempotent)
        if (instance_ != nullptr)
            return;

        spdlog::init_thread_pool(8192, 1); // Queue: 8192, 1 background thread

        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(filename, false);

        instance_ = std::make_unique<spdlog::async_logger>(
            "async_logger",                               // logger ismi
            file_sink,                                    // tek sink
            spdlog::thread_pool(),                        // thread pool (global)
            spdlog::async_overflow_policy::overrun_oldest // queue dolarsa eskiyi at
        );

        instance_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%# %!] %v");
        instance_->set_level(spdlog::level::info);
        instance_->flush_on(spdlog::level::err);      // Hata ve üstü loglar hemen dosyaya yazılır
        spdlog::flush_every(std::chrono::seconds(1)); // Diğerleri ise her 1 saniyede bir flush edilir
    }

    static inline void Info(const spdlog::source_loc &source_loc, const std::string &msg) noexcept
    {
        instance_->log(source_loc, spdlog::level::info, msg);
    }

    static inline void Error(const spdlog::source_loc &source_loc, const std::string &msg) noexcept
    {
        instance_->log(source_loc, spdlog::level::err, msg);
    }

    static inline void Warn(const spdlog::source_loc &source_loc, const std::string &msg) noexcept
    {
        instance_->log(source_loc, spdlog::level::warn, msg);
    }

    static inline void Debug(const spdlog::source_loc &source_loc, const std::string &msg) noexcept
    {
        instance_->log(source_loc, spdlog::level::debug, msg);
    }
};

#define LOG_INFO(msg) Logger::Info(spdlog::source_loc{__FILE__, __LINE__, __FUNCTION__}, msg)
#define LOG_ERROR(msg) Logger::Error(spdlog::source_loc{__FILE__, __LINE__, __FUNCTION__}, msg)
#define LOG_WARN(msg) Logger::Warn(spdlog::source_loc{__FILE__, __LINE__, __FUNCTION__}, msg)
#define LOG_DEBUG(msg) Logger::Debug(spdlog::source_loc{__FILE__, __LINE__, __FUNCTION__}, msg)