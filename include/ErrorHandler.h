#pragma once

#include "GeneratedErrorMap.h"
#include "Logger.h"

#include <cstdio>  // snprintf
#include <cstring> // strerror
#include <cerrno>  // errno
#include <string>  // std::string
#include <cstdint>
#include <thread>
#include <chrono>

using namespace std::chrono;

// 4. ErrorHandler sınıfı
class ErrorHandler
{
private:
    static constexpr uint8_t LOG_BUF_SIZE = 128;
    static constexpr uint16_t MAX_DELAY_MS = 30000;

#ifdef NDEBUG
    static inline uint16_t RETRY_DELAY_MS = 500;
#else
    static inline uint16_t RETRY_DELAY_MS = 0;
#endif

    ErrorHandler() = delete;

    static void BackoffRetry() noexcept
    {
        std::this_thread::sleep_for(milliseconds(RETRY_DELAY_MS));
        RETRY_DELAY_MS = (RETRY_DELAY_MS * 2 >= MAX_DELAY_MS)
                             ? MAX_DELAY_MS
                             : RETRY_DELAY_MS * 2;
    }

    static void logRetry(const char *msg = " ") noexcept
    {
        char buffer[LOG_BUF_SIZE];
        snprintf(buffer, sizeof(buffer), "%s => RETRIED", msg);
        LOG_ERROR(std::string(buffer));
    }

    static void logAbort(const char *msg = " ") noexcept
    {
        char buffer[LOG_BUF_SIZE];
        snprintf(buffer, sizeof(buffer), "%s => ABORTED", msg);
        LOG_ERROR(std::string(buffer));
    }

public:
    static void handleError(int err) noexcept
    {
        switch (errno_strategies[err])
        {
        case ErrorStrategy::Retry:
            logRetry(strerror(err));
            BackoffRetry();
            break;
        case ErrorStrategy::Abort:
            logAbort(strerror(err));
            std::abort();
            break;
        case ErrorStrategy::Custom:
            // will be completed
            break;
        default:
            break;
        }
    }

    static void handleError(ErrorName err) noexcept
    {
        switch (error_strategies[static_cast<int>(err)])
        {
        case ErrorStrategy::Retry:
            logRetry();
            BackoffRetry();
            break;
        case ErrorStrategy::Abort:
            logAbort();
            std::abort();
            break;
        case ErrorStrategy::Custom:
            // will be completed
            break;
        default:
            break;
        }
    }
};
