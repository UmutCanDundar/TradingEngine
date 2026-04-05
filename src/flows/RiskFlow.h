// =============================================================================
// RiskFlow
//
// PURPOSE:
// - Lifecycle controller for the Risk stage execution.
// - Responsible for binding a single Risk component to a dedicated thread.
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

#pragma once

#include "RiskEngine.h"

#include <atomic>
#include <thread>

class RiskFlow
{
private:
    RiskEngine &risk_;
    std::atomic<bool> running_{false};
    std::thread risk_check_thread_;
    std::thread risk_update_thread_;

public:
    RiskFlow(RiskEngine &risk) noexcept;

    RiskFlow(const RiskFlow &) = delete;
    RiskFlow &operator=(const RiskFlow &) = delete;
    RiskFlow(RiskFlow &&) = delete;
    RiskFlow &operator=(RiskFlow &&) = delete;

    void start() noexcept;
    void stop() noexcept;

private:
    void run_check() noexcept;
    void run_update() noexcept;

};