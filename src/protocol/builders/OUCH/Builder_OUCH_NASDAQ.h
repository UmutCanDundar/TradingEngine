// ======================================================================================================
// Builder_OUCH_NASDAQ
//
// PURPOSE:
// - Builds OUCH protocol order messages for NASDAQ venue.
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

#include <cstdint>
#include <cstddef>
#include <array>
#include <cstring>

class SoupBinTcp;

struct Buffer_ONQ
{
    static constexpr size_t MAX_MSG_SIZE = 128;

    char msg[MAX_MSG_SIZE];
    uint16_t len;
};

class Builder_OUCH_NASDAQ
{
private:
    static constexpr size_t DAILY_OUCH_NASDAQ_MSG_COUNT = 65'536;
    std::array<Buffer_ONQ, DAILY_OUCH_NASDAQ_MSG_COUNT> messages;
    size_t next_buffer_slot = 0;
    uint32_t user_ref_num = 1;

    SoupBinTcp &sbt_;

public:
    Builder_OUCH_NASDAQ(SoupBinTcp &sbt) noexcept;

    Buffer_ONQ *build(Order &order) noexcept;  
   
private:
  
    inline char* buildEnterOrder(Order &order, char *buf) noexcept
    {
        *buf = 'O';
        Endian::write_u32_be(buf + 1, user_ref_num);
        *(buf + 5) = order.side == Side::Buy ? 'B' : 'S';
        Endian::write_u32_be(buf + 6, order.quantity);
        std::memcpy(buf + 10, &order.symbol, 8);
        Endian::write_u64_be(buf + 18, static_cast<uint64_t>(order.price));
        *(buf + 26) = static_cast<uint8_t>(order.time_in_force);
        *(buf + 27) = 'Y';
        *(buf + 28) = 'A';
        *(buf + 29) = 'N';
        *(buf + 30) = 'N';
        std::memcpy(buf + 31, &order.client_order_token, 14);
        Endian::write_u16_be(buf + 45, 0);

        order.user_ref_num = user_ref_num++;
    
        return buf;
    }

    inline char* buildReplaceOrder(Order &order, char *buf) noexcept
    {
        *buf = 'U';
        Endian::write_u32_be(buf + 1, order.pre_user_ref_num);
        Endian::write_u32_be(buf + 5, user_ref_num);
        Endian::write_u32_be(buf + 9, order.quantity);
        Endian::write_u64_be(buf + 13, static_cast<uint64_t>(order.price));
        *(buf + 21) = static_cast<uint8_t>(order.time_in_force);
        *(buf + 22) = 'Y';
        *(buf + 23) = 'N';
        std::memcpy(buf + 24, &order.client_order_token, 14);
        Endian::write_u16_be(buf + 38, 0);

        order.pre_user_ref_num = order.user_ref_num;
        order.user_ref_num = user_ref_num++;
    
        return buf;
    }

    inline char* buildCancelOrder(Order &order, char *buf) noexcept
    {
        *buf = 'X';
        Endian::write_u32_be(buf + 1, user_ref_num);
        Endian::write_u32_be(buf + 5, order.quantity);
        Endian::write_u16_be(buf + 9, 0);

        return buf;
    }

    inline char* buildModifyOrder(Order &order, char *buf) noexcept
    {
        *buf = 'M';
        Endian::write_u32_be(buf + 1, user_ref_num);
        *(buf + 5) = order.side == Side::Buy ? 'B' : 'S';
        Endian::write_u32_be(buf + 6, order.quantity);
        Endian::write_u16_be(buf + 10, 0);

        return buf;
    }

    inline char *buildAccountQueryRequest([[maybe_unused]] Order &order, char *buf) noexcept
    {
        *buf = 'Q';
        Endian::write_u16_be(buf + 1, 0);

        return buf;
    }

    using OuchHandlerFunc = char *(Builder_OUCH_NASDAQ::*)(Order & order, char *buf) noexcept;
    static constexpr std::array<OuchHandlerFunc, 5> OuchMessageBuilders =
    {
            &Builder_OUCH_NASDAQ::buildEnterOrder,
            &Builder_OUCH_NASDAQ::buildReplaceOrder,
            &Builder_OUCH_NASDAQ::buildCancelOrder,
            &Builder_OUCH_NASDAQ::buildModifyOrder,
            &Builder_OUCH_NASDAQ::buildAccountQueryRequest,
    };

};

