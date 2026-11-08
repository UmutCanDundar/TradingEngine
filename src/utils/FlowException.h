// ======================================================================================================
// ThreadFailure
//
// PURPOSE:
// - Carries the FIRST fatal exception of any worker (flow) thread to the main thread.
// - Worker threads catch everything at their entry point and call g_thread_failure.report(...).
// - Main polls has_failed(); when true it rethrows the stored exception in its own try/catch,
//   logs it, and performs the normal graceful shutdown.
//
// THREAD SAFETY:
// - report() may be called concurrently by several threads; only the first one stores its exception
//   (std::call_once), the others return without touching `error` (no data race).
// - `error` must only be read by main AFTER has_failed() returned true (release/acquire pairing).
// - Not on the hot path: report() runs once, in the catch block of a dying thread.
//
// DEVELOPER NOTES:
// - `inline` variable: exactly one instance across all translation units. Do NOT make it `static`.
// ======================================================================================================
#pragma once

#include <atomic>
#include <exception>
#include <mutex>
#include <utility>

struct ThreadFailure
{
    std::once_flag once;
    std::atomic<bool> failed{false};
    std::exception_ptr error;

    void report(std::exception_ptr e) noexcept
    {
        std::call_once(once, [&] {
            error = std::move(e);
            failed.store(true, std::memory_order_release);   
        });
    }

    bool has_failed() const noexcept { return failed.load(std::memory_order_acquire); }
};

inline ThreadFailure thread_failure;