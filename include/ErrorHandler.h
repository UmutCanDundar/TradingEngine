#pragma once

#include "GeneratedErrorMap.h"

#include <cstdint>

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

    static void BackoffRetry() noexcept;

    static void logRetry(const char *msg = " ") noexcept;

    static void logAbort(const char *msg = " ") noexcept;

public:
    static void handleError(int err) noexcept;

    static void handleError(ErrorName err) noexcept;
};
