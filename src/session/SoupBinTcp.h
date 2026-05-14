// =========================================================================================================
// SoupBinTcp
//
// PURPOSE:
// - Handles the SoupBinTCP (SBT) protocol for a trading venue, including login, heartbeat,
//   logout, and packet dispatching to appropriate handlers.
// - Provides fast hot-path access for per-message packet handling and header construction.
//
// THREAD SAFETY:
// - Designed for single-thread ownership per SoupBinTcp instance.
// - No internal atomics are used; correctness relies on strict session-thread binding.
//
// PERFORMANCE & DESIGN NOTES:
// - Hot-path methods and packet handlers are inline for minimal call overhead. While, cold methods are
//   implemented in the .cpp to reduce compile-time dependencies.
// - Session buffers (sess_bufs) are implemented as a fixed-size ring buffer, cache-friendly, and
//   sized to accommodate 64 sessions. Historical packet retention is not required, so old entries
//   are overwritten when buffers wrap around.
// - Switch-case was chosen over function pointer tables or templates because, for a small fixed set of
//   7 packet types, it allows each handler to be fully inlined, minimizing call overhead and maximizing
//   branch predictor efficiency on the hot path while keeping the code simple and readable.
// - Switch-case layout ensures that the most frequent packet type ('S') is first, reducing
//   misprediction penalty.
//
// DEVELOPER NOTES (PRE-PROFILING):
// - Memory footprint: ~4.36 KB per SoupBinTcp instance. Currently part of the main TradingEngine object;
//   lifetime is long-lived. Even if stack usage is small (~4 KB), heap allocation is preferred
//   to accommodate future architectural changes where this class might be separated into a dedicated 
//   pipeline while maintaining long-lived lifetime semantics.
// =========================================================================================================

#pragma once

#include "Endian.h"
#include "SessionManager.h"

#include <array>
#include <cstdint>
#include <cstddef>

enum class SBTAction : uint8_t {
    NotToParser = 0,
    ToParser = 1,
    Disconnect = 2,
    Reconnect = 3,
    Ignore = 4,
};

class SoupBinTcp
{
private:
  
    SessionManager &sess_mngr_;
    
public:
    SoupBinTcp(SessionManager &sess_mngr) noexcept;

    SBTAction pkt_handler(const char *pkt, size_t sess_index) noexcept;
    char *buildFirstLoginRequest(char *buf, uint8_t sess_index) noexcept;
    char *buildReLoginRequest(char *buf, uint8_t sess_index) noexcept;
    char *buildLogoutRequest(char *buf) noexcept;

    inline char *buildHeartbeat(char *buf) noexcept
    {
        Endian::write_u16_be(buf, 1);
        *(buf + 2) = 'R';

        return buf;
    }

    inline char* WriteSBTHeaderForDataPacket(char *buf, uint16_t OUCHpkt_len) noexcept
    {
        Endian::write_u16_be(buf, OUCHpkt_len - 2);
        *(buf + 2) = 'U';

        return buf + 3;
    }

private:
    __attribute__((always_inline)) 
    inline SBTAction handleSequencedData(size_t sess_index) noexcept
    {
        auto &seq_sbt = sess_mngr_.getSessionState(sess_index)->sbt;
        seq_sbt.increase_seq();
        return SBTAction::ToParser;
    }
    
    __attribute__((always_inline)) 
    inline SBTAction handleHeartbeat() noexcept
    { 
        return SBTAction::Ignore; 
    }

    __attribute__((always_inline)) 
    inline SBTAction handleEndOfSession() noexcept
    { 
        return SBTAction::Reconnect; 
    }

    __attribute__((always_inline)) 
    inline SBTAction handleLoginAccepted(const char *LoginAcceptedPacket, size_t sess_index) noexcept
    {
        auto* state = sess_mngr_.getSessionState(sess_index);
        state->is_logged_in.store(true, std::memory_order_release);
        state->logged_before.store(true, std::memory_order_release);
        state->sbt.set_sess_id(LoginAcceptedPacket + 3);
        state->sbt.set_seq(LoginAcceptedPacket + 13);
        return SBTAction::NotToParser;
    }
    
    __attribute__((always_inline))
    inline SBTAction handleLoginRejected(const char *LoginRejectedPacket) noexcept
    {
        if (LoginRejectedPacket[3] == 'S') // Session not available
            return SBTAction::Reconnect;
        else // Not Authorized
            return SBTAction::Disconnect;
    }
    
    __attribute__((always_inline)) 
    inline SBTAction handleUnSequencedData() noexcept
    { 
        return SBTAction::NotToParser; 
    }
    
    __attribute__((always_inline)) 
    inline SBTAction handleDebug() noexcept
    { 
        return SBTAction::Ignore; 
    }

};

