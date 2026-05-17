// =======================================================================================================
// LoginController
//
// PURPOSE:
// - Centralizes login attempt orchestration and retry/backoff decisions for all session protocols.
// - Abstracts protocol-specific login message construction (SoupBinTCP vs FIX).
// - Enforces retry limits, backoff timing, and session state validation before login attempts.
//
// THREAD SAFETY:
// - This class itself does not own synchronization primitives; correctness relies on
//   SessionState atomic semantics and external session lifecycle guarantees.
//
// PERFORMANCE & DESIGN NOTES:
// - This is a cold-path component; login logic is not latency-critical and not on the trading hot path.
// - ALLOWED_LOGIN_ATTEMPT limits the maximum number of consecutive login retries per session.
// - MIN_LOGIN_INTERVAL_NS and MAX_LOGIN_INTERVAL_NS define exponential backoff bounds for login attempts.
// - Backoff uses wall-clock nanosecond timing and may invoke sleep_for. While non-blocking alternatives
//   exist (e.g. epoll/timerfd-driven retries or event-loop based scheduling), they were intentionally
//   avoided here to keep the login path simple and explicit.
//   Given that login attempts are infrequent and off the hot path, temporarily blocking the control
//   thread is acceptable. This decision may be revisited if login orchestration becomes more complex or
//   higher-frequency.
//
// DEVELOPER NOTES (PRE-PROFILING):
// - Retry/backoff strategy may be refined after observing real exchange behavior.
// - If login traffic increases or becomes contention-heavy, protocol-specific controllers
//   may be split out.
// - Login backoff is currently applied per-session and per-socket. If a session drops, retry attempts 
//   are serialized on the same socket. A more advanced failover model (e.g. maintaining a warm standby 
//   socket or allowing immediate login continuation on an alternate connection) was considered but 
//   intentionally deferred.
//   This avoids premature complexity in session orchestration and state synchronization. Such mechanisms 
//   can be introduced later if operational requirements demand faster recovery or higher availability.
// =======================================================================================================

#pragma once

#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <time.h>

class SessionManager;
class SoupBinTcp;
class Builder_FIX;
struct SessionState;
struct TxPacket;

enum class LoginDecision : uint8_t 
{
    TryAgain = 0,
    Disconnect = 1,
    AlreadyLoggedIn = 2,
    Try = 3
};

inline constexpr uint8_t ALLOWED_LOGIN_ATTEMPT = 3;

class LoginController
{
private:
    static constexpr uint64_t MIN_LOGIN_INTERVAL_NS = 5'000'000'000ULL;
    static constexpr uint64_t MAX_LOGIN_INTERVAL_NS = 60'000'000'000ULL;

    SoupBinTcp& sbt_;
    Builder_FIX& builder_fix_;
    SessionManager& sess_mngr_;

public:
    LoginController(SoupBinTcp &sbt, Builder_FIX &builder_fix, SessionManager &sess_mngr) noexcept;
    
    void LoginAttempt(TxPacket& login_pkt, const uint8_t index, SessionState& state) noexcept;
    LoginDecision LoginDecider(SessionState* state) noexcept;
   
    static inline uint64_t NowNs() noexcept
    {
        timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return uint64_t(ts.tv_sec) * 1'000'000'000ULL + ts.tv_nsec;
    }

private:
   
    static inline uint64_t compute_backoff_ns(uint32_t retry_count) noexcept
    {
        uint64_t interval = MIN_LOGIN_INTERVAL_NS << retry_count;

        return std::min(interval, MAX_LOGIN_INTERVAL_NS);            
    }
};

 