// =============================================================================
// StrategyFlow
//
// PURPOSE:
// - Lifecycle controller for the Strategy stage execution.
// - Responsible for binding a single Strategy component to a dedicated thread.
// - Provides explicit start/stop semantics for controlled execution.
//
// THREADING MODEL:
// - Owns exactly one worker thread.
// - Thread lifetime is fully managed by this class.
// - Uses an atomic run flag for cooperative shutdown.
//
// THREAD SAFETY:
// - Non-copyable and non-movable by design.
// - Guarantees single ownership of the underlying thread.
// - Safe under the assumption that start/stop are not called concurrently.
//
// PERFORMANCE CHARACTERISTICS:
// - Cold-path component (initialized during system startup).
// - No work performed on the hot path; only dispatch loop coordination.
// - Busy-wait mitigation via `_mm_pause()` when no work is available.
//
// SYSTEM CONFIGURATION:
// - Applies memory locking (mlockall) to avoid page faults.
// - Configures real-time scheduling policy (SCHED_FIFO).
//
// DEVELOPER NOTES:
// - Must not be copied or moved; thread affinity and atomic state depend on
//   stable object address.
// - All Flow classes in the system follow the same structural and lifecycle model.
// =============================================================================

// #pragma once

// #include "Strategy.h"

// #include <atomic>
// #include <thread>

// class StrategyFlow
// {
// private:
//     Strategy &strategy_;
//     std::atomic<bool> running_{false};
//     std::thread strategy_thread_;

// public:
//     StrategyFlow(Strategy &strategy) noexcept;

//     StrategyFlow(const StrategyFlow &) = delete;
//     StrategyFlow &operator=(const StrategyFlow &) = delete;
//     StrategyFlow(StrategyFlow &&) = delete;
//     StrategyFlow &operator=(StrategyFlow &&) = delete;

//     void start() noexcept;
//     void stop() noexcept;

// private:
//     void run() noexcept;

// };