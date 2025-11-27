#pragma once

#include "GeneratedErrorMap.h"
#include "Logger.h"

#include <cstdint>
#include <string_view>

class ErrorHandler
{
private:
    static constexpr uint8_t LOG_BUF_SIZE = 128;
    static constexpr uint16_t MAX_DELAY_MS = 30000;

#ifdef NDEBUG
    static inline uint16_t START_RETRY_DELAY_MS = 500;
#else
    static inline uint16_t START_RETRY_DELAY_MS = 0;
#endif

    static inline uint16_t RETRY_DELAY_MS = START_RETRY_DELAY_MS;   
    static inline uint8_t retry_count = 0;

    ErrorHandler() = delete;

    static bool BackoffRetry(uint8_t max_retry) noexcept;
    
    static void resetRetry() noexcept
    {
        RETRY_DELAY_MS = START_RETRY_DELAY_MS;
        retry_count = 0;
    }

    template <typename... Args>
    static void logError(std::string_view msg, Args &&...args) noexcept
    {
        LOG_ERROR(msg, std::forward<Args>(args)...);
    }

public:
    static bool handleError(int err, uint8_t max_retry = 3) noexcept;

    static void handleError(ErrorName err) noexcept;
};
