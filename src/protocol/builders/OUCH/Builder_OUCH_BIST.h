// ======================================================================================================
// Builder_OUCH_BIST
//
// PURPOSE:
// - Builds OUCH protocol order messages for BIST venue.
//   Responsible for transforming internal Order objects into wire-ready
//   OUCH binary messages, including SoupBinTCP framing.
//
// THREAD SAFETY:
// - Not thread-safe by design.
// - Intended to be used from a single producer thread.
//
// PERFORMANCE & DESIGN NOTES:
// - Uses a fixed-size preallocated message pool to avoid dynamic allocations.
// - Dispatch is performed via a function table indexed by message_type,
//   providing branchless, predictable dispatch behavior.
// - Account-specific static fields (client_account, exchange_info) are
//   prefilled once at construction time to avoid redundant per-message writes.
// - Message builders operate directly on raw buffers to minimize abstraction
//   overhead and maximize data locality.
// - Endianness conversions are explicit and localized.
//
// DEVELOPER NOTES (PRE-PROFILING):
// - This class must be created on the heap due to ~8.5MB Stack usage.
//
// - The builder path is largely memory-bound due to sequential buffer writes, multiple memcpy/memset
//   operations, and fixed-layout binary message construction; therefore, forcing inline or constexpr
//   dispatch upfront is not expected to yield meaningful latency improvements.
//   Dispatch is currently implemented via a function table for correctness and clarity. Since the number
//   of message types is small, this can be revisited after profiling: hot paths may be converted to
//   switch-case dispatch to enable inlining and reduce instruction count, provided instruction cache
//   pressure remains acceptable.
//
// - Message builders are marked inline for clarity and future flexibility;
//   however, due to function-pointer-based dispatch, they are not expected
//   to inline in the current design.
//  
// - After observing real traffic patterns, message-type–specific prefilled templates may be introduced
//   to further reduce runtime instruction count. In such a design, invariant fields would be written
//   once at initialization time, and only truly dynamic fields (e.g., quantity, price, order IDs)
//   would be patched per message, trading memory for lower instruction and store pressure on hot paths.
// ======================================================================================================

#pragma once

#include "Order.h"
#include "Endian.h"
#include "SessionManager.h"

#include <cstdint>
#include <cstddef>
#include <array>
#include <cstring>
#include <string_view>

class SoupBinTcp;

struct ExchangeClientInfo_local 
{
    char client_account[16]; // Client/Account for DM
    char exchange_info[32];  // Account number (first 16 used) for EQM

    ExchangeClientInfo_local() = default;

    ExchangeClientInfo_local(const std::string_view cl_acc, const std::string_view exc_info) 
    {
        std::memset(client_account, ' ', 16);
        std::memset(exchange_info, ' ', 32);
        
        std::memcpy(client_account, cl_acc.data(), std::min(cl_acc.size(), size_t(16)));
        std::memcpy(exchange_info, exc_info.data(), std::min(exc_info.size(), size_t(32)));
    }
};

struct Buffer_OBT // May be separated per msg type after observing real traffic to avoid unnecessary set to some fields
{
    static constexpr size_t MAX_MSG_SIZE = 128;

    char msg[MAX_MSG_SIZE];
    uint16_t len;

   Buffer_OBT() { std::memset(msg, ' ', MAX_MSG_SIZE); }
};

class Builder_OUCH_BIST
{
private:
    static constexpr size_t DAILY_OUCH_BIST_MSG_COUNT = 65'536;
    std::array<Buffer_OBT, DAILY_OUCH_BIST_MSG_COUNT> messages;
    size_t next_buffer_slot = 0;

    const std::array<ExchangeClientInfo_local, ACC_COUNT> eci_local_;

    SoupBinTcp &sbt_;

public:
    Builder_OUCH_BIST(SoupBinTcp &sbt, SessionManager &sess_mngr) noexcept;

    Buffer_OBT *build(const Order &order) noexcept;
   
private:
    std::array<ExchangeClientInfo_local, ACC_COUNT> initialize_exc_info(SessionManager &sess_mngr) noexcept;
    

    inline char* buildEnterOrder(const Order &order, char *buf) noexcept
    {
        *buf = 'O';
        std::memcpy(buf + 1, &order.client_order_token, order.real_cl_ord_token_len);
        Endian::write_u32_be(buf + 15, order.instrument_id);
        *(buf + 19) = order.side == Side::Buy ? 'B' : 'S';
        Endian::write_u64_be(buf + 20, order.quantity);
        Endian::write_i32_be(buf + 28, order.price);
        *(buf + 32) = static_cast<uint8_t>(order.time_in_force);
        *(buf + 33) = 0;
        std::memcpy(buf + 34, eci_local_[order.account_index].client_account, 16);
        // std::memset(buf + 50, ' ', 15);
        std::memcpy(buf + 65, eci_local_[order.account_index].exchange_info, 32);
        Endian::write_u64_be(buf + 97, 0);
        *(buf + 105) = 1;
        *(buf + 106) = 0;
        *(buf + 107) = 0;
        *(buf + 108) = 0;
        std::memset(buf + 109, ' ', 3);
        std::memset(buf + 112, 0, 2);

        return buf;
    }

    inline char* buildReplaceOrder(const Order &order, char *buf) noexcept
    {
        *buf = 'U';
        std::memcpy(buf + 1, &order.fix_org_order_id, 14);
        std::memcpy(buf + 15, &order.client_order_token, order.real_cl_ord_token_len);
        Endian::write_u64_be(buf + 29, order.quantity);
        Endian::write_i32_be(buf + 37, order.price);
        *(buf + 41) = 0;
        std::memcpy(buf + 42, eci_local_[order.account_index].client_account, 16);
        // std::memset(buf + 58, ' ', 15);
        std::memcpy(buf + 73, eci_local_[order.account_index].exchange_info, 32);
        Endian::write_u64_be(buf + 105, 0);
        *(buf + 113) = 1;
        std::memset(buf + 114, 0, 8);

        return buf;
    }

    inline char* buildCancelOrder(const Order &order, char *buf) noexcept
    {
        *buf = 'X';
        std::memcpy(buf + 1, &order.client_order_token, 14);
      
        return buf;
    }

    inline char* buildCancelByOrderID(const Order &order, char *buf) noexcept
    {
        *buf = 'Y';
        Endian::write_u32_be(buf + 1, order.instrument_id);
        *(buf + 5) = order.side == Side::Buy ? 'B' : 'S';
        Endian::write_u64_be(buf + 6, order.order_id);

        return buf;
    }
    
    using OuchHandlerFunc = char* (Builder_OUCH_BIST::*)(const Order &order, char *buf) noexcept;
    static constexpr std::array<OuchHandlerFunc, 4> OuchMessageBuilders =
    {
            &Builder_OUCH_BIST::buildEnterOrder,
            &Builder_OUCH_BIST::buildReplaceOrder,
            &Builder_OUCH_BIST::buildCancelOrder,
            &Builder_OUCH_BIST::buildCancelByOrderID
    };
};

