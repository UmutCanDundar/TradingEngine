// ======================================================================================================
// ErrorHandler
//
// PURPOSE:
//  - Centralized mechanism for handling system and network errors, including errno-based and
//    application-specific errors (`ErrorName`).
//  - Implements automatic retry with exponential backoff for recoverable errors, up to a
//    configurable maximum delay and retry count.
//  - Logs errors to the `Logger` class for cold-path diagnostics and auditing.
//
// THREAD SAFETY:
// - `thread_local` counters (`errno_retry_count`, `error_retry_count`) ensure retry state
//   is isolated per thread, preventing conflicts when multiple threads invoke ErrorHandler.
// - Retry counters are thread-local; multiple threads can call ErrorHandler concurrently
//   without interfering with each other's retry state. The retryable action itself must be
//   managed by the caller, not by ErrorHandler.
//
// PERFORMANCE & DESIGN NOTES:
//  - Designed for **cold path usage**; hot-path trading or network operations should not
//    call ErrorHandler frequently.
//  - Logging is delegated to `Logger`, which is optimized for infrequent, cold-path messages.
//  - Two separate retry mechanisms: one for `errno` system/network errors, one for application
//    `ErrorName` errors, to avoid state collision.
//  - Exponential backoff caps at `MAX_DELAY_MS` (30s by default), starts from `START_RETRY_DELAY_MS`.
//  - `BackoffRetry_*` functions sleep the thread using `std::this_thread::sleep_for`, ensuring
//    the retry is blocking but controlled.
//  - `handleError(int)` and `handleError(ErrorName)` handle `Retry` and `Abort` strategies;
//    `Abort` triggers `std::abort()` for unrecoverable errors.
//
// DEVELOPER NOTES:
//  - All methods are static; `ErrorHandler` cannot be instantiated.
//  - Template-based logging `logError` ensures variadic formatting works with cold-path logging.
//  - Designed to be robust against repeated errors on the same thread without interfering with
//    other threads or socket/session error handling.
//=======================================================================================================

#pragma once

#include "Logger.h"
#include "GeneratedErrorMap.h"

#include <cstdint>
#include <string_view>
#include <utility>

enum class ErrorName : uint8_t;

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

    static inline thread_local uint16_t errno_RETRY_DELAY_MS = START_RETRY_DELAY_MS;   
    static inline thread_local uint8_t errno_retry_count = 0;

    static inline thread_local uint16_t error_RETRY_DELAY_MS = START_RETRY_DELAY_MS;
    static inline thread_local uint8_t error_retry_count = 0;

    ErrorHandler() = delete;

    static bool BackoffRetry_errno(uint8_t max_retry) noexcept;
    static bool BackoffRetry_error(uint8_t max_retry) noexcept;
    static void resetRetry_errno() noexcept;
    static void resetRetry_error() noexcept;
  
    template <typename... Args>
    static void logError(std::string_view msg, Args &&...args) noexcept
    {
        LOG_ERROR(msg, std::forward<Args>(args)...);
    }

    static constexpr std::string_view toString(ErrorName err) noexcept 
    {
        using enum ErrorName;
        switch (err) 
        {
            case InvalidIP:          return "InvalidIP";
            case CouldNotOpenFile:   return "CouldNotOpenFile";
            default:                 return "UnknownError";
        }
    }

public:
    static bool handleError(int err, uint8_t max_retry = 3) noexcept;
    static bool handleError(ErrorName err, uint8_t max_retry = 3) noexcept;
};
