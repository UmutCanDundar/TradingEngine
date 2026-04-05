#include "LoginController.h"
#include "SoupBinTcp.h"
#include "Builder_FIX.h"
#include "NetworkPackets.h"
#include "SessionManager.h"

#include <thread>
#include <chrono>
#include <cstring>

LoginController::LoginController(SoupBinTcp &sbt, Builder_FIX &builder_fix, SessionManager &sess_mngr) noexcept : sbt_(sbt), builder_fix_(builder_fix), sess_mngr_(sess_mngr) {}

void LoginController::LoginAttempt(InPacket &login_pkt, const uint8_t index, SessionState &state) noexcept
{
    auto sess_prot = state.sess_prot;

    if (sess_prot == SessionProtocol::SBT)
    {
        if (!sess_mngr_.isSessionLoggedInBefore(index))
            sbt_.buildFirstLoginRequest(login_pkt.data.data(), index);
        else
            sbt_.buildReLoginRequest(login_pkt.data.data(), index);
    }
    else
    {
        Buffer_FIX *buffer;

        if (!sess_mngr_.isSessionLoggedInBefore(index))
            buffer = builder_fix_.build<FIXTypes::Logon>(index, true);
        else
            buffer = builder_fix_.build<FIXTypes::Logon>(index, false);

        std::memcpy(buffer->data, login_pkt.data.data(), buffer->len);
        login_pkt.len = buffer->len;
        login_pkt.sock_index = index;
    }

    state.last_login_attempt_ns = NowNs();
    state.login_retry_count++;
}

LoginDecision LoginController::LoginDecider(SessionState *state) noexcept
{
    if (!state->is_connected.load(std::memory_order_relaxed))
        return LoginDecision::Disconnect;

    if (state->is_logged_in.load(std::memory_order_acquire))
        return LoginDecision::AlreadyLoggedIn;

    if (state->login_retry_count >= ALLOWED_LOGIN_ATTEMPT)
        return LoginDecision::Disconnect;

    uint64_t backoff_ns = compute_backoff_ns(state->login_retry_count);
    uint64_t elapsed_ns = NowNs() - state->last_login_attempt_ns;
    if (elapsed_ns < backoff_ns)
        std::this_thread::sleep_for(std::chrono::nanoseconds(backoff_ns - elapsed_ns));

    return LoginDecision::TryAgain;
}