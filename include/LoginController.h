#pragma once

#include "SoupBinTcp.h"
#include "Builder_FIX.h"
#include "NetworkPackets.h"
#include "SessionManager.h"

#include <time.h>

enum class LoginDecision : uint8_t 
{
    TryAgain,
    // WaitBackoff,
    Stop,
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

    inline void LoginAttempt(InPacket& login_pkt, const int sock, const uint8_t index, SessionState& state) noexcept
    {
        state.last_login_attempt_ns = NowNs();

        auto sess_prot = state.sess_prot;

        if(sess_prot == SessionProtocol::SBT)
        {
            if (!sess_mngr_.isSessionLoggedInBefore(index))
                sbt_.buildFirstLoginRequest(login_pkt.data.data(), index);
            else
                sbt_.buildReLoginRequest(login_pkt.data.data(), index);
        }
        else
        {
            Buffer_FIX* buffer;

            if (!sess_mngr_.isSessionLoggedInBefore(index))
                buffer = builder_fix_.build<FIXTypes::Logon>(index, true);
            else
                buffer = builder_fix_.build<FIXTypes::Logon>(index, false);

              std::memcpy(buffer->data, login_pkt.data.data(), buffer->len);
              login_pkt.len = buffer->len;
              login_pkt.sock_index = index;
        }
    }

    inline LoginDecision LoginDecider(const uint8_t index) noexcept
    {
        SessionState* state = sess_mngr_.getSessionState(index);

        if (!state->is_connected.load(std::memory_order_relaxed))
            return LoginDecision::Stop;

        if (state->is_logged_in.load(std::memory_order_acquire))
            return LoginDecision::Stop;

        if (state->login_retry_count >= ALLOWED_LOGIN_ATTEMPT)
            return LoginDecision::Stop;

        ++state->login_retry_count;

        uint64_t backoff_ns = compute_backoff_ns(state->login_retry_count);
        uint64_t elapsed_ns = NowNs() - state->last_login_attempt_ns; 
        if (elapsed_ns < backoff_ns) 
            std::this_thread::sleep_for(std::chrono::nanoseconds(backoff_ns - elapsed_ns));   

        return LoginDecision::TryAgain;
    }

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

 