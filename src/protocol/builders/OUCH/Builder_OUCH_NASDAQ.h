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
// - The builder path is largely memory-bound due to sequential buffer writes, multiple memcpy/memset
//   operations, and fixed-layout binary message construction; therefore, forcing inline or constexpr
//   dispatch upfront is not expected to yield meaningful latency improvements.
//   Dispatch is currently implemented via a function table for correctness and clarity. Since the number
//   of message types is small, this can be revisited after profiling: hot paths may be converted to
//   switch-case dispatch to enable inlining and reduce instruction count, provided instruction cache
//   pressure remains acceptable.(*)TESTED-see benchmark reports
//
// - Message builders are marked inline for clarity and future flexibility;
//   however, due to function-pointer-based dispatch, they are not expected
//   to inline in the current design.
//
// - After observing real traffic patterns, message-type–specific prefilled templates may be introduced
//   to further reduce runtime instruction count. In such a design, invariant fields would be written
//   once at initialization time, and only truly dynamic fields (e.g., quantity, price, order IDs)
//   would be patched per message, trading memory for lower instruction and store pressure on hot paths.(*)TESTED-see benchmark reports
// ======================================================================================================

//FUNC_PTR NO-PRE-FILL

#pragma once

#include "Order.h"
#include "Endian.h"

#include <cstdint>
#include <cstddef>
#include <array>
#include <cstring>

class SoupBinTcp;

struct Buffer_ONQ // May be separated per msg type after observing real traffic to avoid unnecessary set to all fields
{
    static constexpr size_t MAX_MSG_SIZE = 126;

    char msg[MAX_MSG_SIZE];
    uint16_t len;

    Buffer_ONQ() { std::memset(msg, ' ', MAX_MSG_SIZE); }

};

class Builder_OUCH_NASDAQ
{
private:
    static constexpr size_t DAILY_OUCH_NASDAQ_MSG_COUNT = 2;    
    std::array<Buffer_ONQ, DAILY_OUCH_NASDAQ_MSG_COUNT> messages;
    size_t next_buffer_slot = 0;
    uint32_t user_ref_num = 1;

    SoupBinTcp &sbt_;

public:
    Builder_OUCH_NASDAQ(SoupBinTcp &sbt) noexcept;

    Buffer_ONQ *build(Order &order) noexcept;  
   
private:
  
    inline void buildEnterOrder(Order &order, char *buf) noexcept
    {
        *buf = 'O';
        Endian::write_u32_be(buf + 1, user_ref_num);
        *(buf + 5) = order.side == Side::Buy ? 'B' : 'S';
        Endian::write_u32_be(buf + 6, order.quantity);
        std::memcpy(buf + 10, &order.symbol, order.real_symbol_len);
        Endian::write_u64_be(buf + 18, static_cast<uint64_t>(order.price));
        *(buf + 26) = static_cast<uint8_t>(order.time_in_force);
        *(buf + 27) = 'Y';
        *(buf + 28) = 'A';
        *(buf + 29) = 'N';
        *(buf + 30) = 'N';
        std::memcpy(buf + 31, &order.client_order_token, order.real_cl_ord_token_len);
        Endian::write_u16_be(buf + 45, 0);

        order.user_ref_num = user_ref_num++;
    
    }

    inline void buildReplaceOrder(Order &order, char *buf) noexcept
    {
        *buf = 'U';
        Endian::write_u32_be(buf + 1, order.pre_user_ref_num);
        Endian::write_u32_be(buf + 5, user_ref_num);
        Endian::write_u32_be(buf + 9, order.quantity);
        Endian::write_u64_be(buf + 13, static_cast<uint64_t>(order.price));
        *(buf + 21) = static_cast<uint8_t>(order.time_in_force);
        *(buf + 22) = 'Y';
        *(buf + 23) = 'N';
        std::memcpy(buf + 24, &order.client_order_token, order.real_cl_ord_token_len);
        Endian::write_u16_be(buf + 38, 0);

        order.pre_user_ref_num = order.user_ref_num;
        order.user_ref_num = user_ref_num++;
    
    }

    inline void buildCancelOrder(Order &order, char *buf) noexcept
    {
        *buf = 'X';
        Endian::write_u32_be(buf + 1, user_ref_num);
        Endian::write_u32_be(buf + 5, order.quantity);
        Endian::write_u16_be(buf + 9, 0);

    }

    inline void buildModifyOrder(Order &order, char *buf) noexcept
    {
        *buf = 'M';
        Endian::write_u32_be(buf + 1, user_ref_num);
        *(buf + 5) = order.side == Side::Buy ? 'B' : 'S';
        Endian::write_u32_be(buf + 6, order.quantity);
        Endian::write_u16_be(buf + 10, 0);

    }

    inline void buildAccountQueryRequest([[maybe_unused]] Order &order, char *buf) noexcept
    {
        *buf = 'Q';
        Endian::write_u16_be(buf + 1, 0);

    }

    using OuchHandlerFunc = void (Builder_OUCH_NASDAQ::*)(Order & order, char *buf) noexcept;
    static constexpr std::array<OuchHandlerFunc, 5> OuchMessageBuilders =
    {
            &Builder_OUCH_NASDAQ::buildEnterOrder,
            &Builder_OUCH_NASDAQ::buildReplaceOrder,
            &Builder_OUCH_NASDAQ::buildCancelOrder,
            &Builder_OUCH_NASDAQ::buildModifyOrder,
            &Builder_OUCH_NASDAQ::buildAccountQueryRequest,
    };

};



//FUNC_PTR PRE-FILL

// #pragma once

// #include "Order.h"
// #include "Endian.h"

// #include <cstdint>
// #include <cstddef>
// #include <array>
// #include <cstring>

// class SoupBinTcp;

// struct Buffer_ONQ
// {
//     char*    msg;
//     uint16_t len; //SBT HEADER + MSG SIZE
// };

// struct Buffer_ONQ_Enter : Buffer_ONQ
// {
//     static constexpr size_t ENTER_SIZE = 47;

//     char data[ENTER_SIZE];
//     Buffer_ONQ_Enter() : Buffer_ONQ{data, ENTER_SIZE}
//     { 
//         std::memset(data, ' ', ENTER_SIZE);
//         data[3]  = 'O';
//         data[30] = 'Y';
//         data[31] = 'A';
//         data[32] = 'N';
//         data[33] = 'N';
//         Endian::write_u16_be(data + 48, 0); 
//     }
// };

// struct Buffer_ONQ_Replace : Buffer_ONQ
// {
//     static constexpr size_t REPLACE_SIZE = 40;

//     char data[REPLACE_SIZE];
//     Buffer_ONQ_Replace() : Buffer_ONQ{data, REPLACE_SIZE}
//     { 
//         std::memset(data, ' ', REPLACE_SIZE); 
//         data[3]  = 'U';
//         data[25] = 'Y';
//         data[26] = 'N';
//         Endian::write_u16_be(data + 41, 0);
//     }
// };

// struct Buffer_ONQ_Cancel : Buffer_ONQ
// {
//     static constexpr size_t CANCEL_SIZE = 11;

//     char data[CANCEL_SIZE];
//     Buffer_ONQ_Cancel() : Buffer_ONQ{data, CANCEL_SIZE}
//     { 
//         std::memset(data, ' ', CANCEL_SIZE); 
//         data[3] = 'X';
//         Endian::write_u16_be(data + 12, 0);
//     }
// };

// struct Buffer_ONQ_Modify : Buffer_ONQ
// {
//     static constexpr size_t MODIFY_SIZE = 12;

//     char data[MODIFY_SIZE];
//     Buffer_ONQ_Modify() : Buffer_ONQ{data, MODIFY_SIZE}
//     { 
//         std::memset(data, ' ', MODIFY_SIZE);
//         data[3] = 'M';
//         Endian::write_u16_be(data + 13, 0); 
//     }
// };

// struct Buffer_ONQ_Query : Buffer_ONQ
// {
//     char data[3];
//     Buffer_ONQ_Query() : Buffer_ONQ{data, 3}
//     {
//         std::memset(data, ' ', 3);
//         data[3] = 'Q';
//         Endian::write_u16_be(data + 4, 0);
//     }
// };

// class Builder_OUCH_NASDAQ
// {
// private:
//     static constexpr size_t DAILY_ENTER = 1 << 1;                /* 1 << 15; // 32'768 */
//     static constexpr size_t DAILY_REPLACE = 1 << 1;              /* 1 << 14; // 16'384 */
//     static constexpr size_t DAILY_CANCEL = 1 << 1;               /* 1 << 13; // 8'192  */
//     static constexpr size_t DAILY_MODIFY = 1 << 1;               /* 1 << 13; // 8'192  */
//     static constexpr size_t DAILY_ACCQUERY = 1 << 1;        

//     std::array<Buffer_ONQ_Enter, DAILY_ENTER> enter_messages;
//     std::array<Buffer_ONQ_Replace, DAILY_REPLACE> replace_messages;
//     std::array<Buffer_ONQ_Cancel, DAILY_CANCEL> cancel_messages;
//     std::array<Buffer_ONQ_Modify, DAILY_MODIFY> modify_messages;
//     std::array<Buffer_ONQ_Query, DAILY_ACCQUERY> query_messages;
    
//     size_t next_enter = 0;
//     size_t next_replace = 0;
//     size_t next_cancel = 0;
//     size_t next_modify = 0;
//     size_t next_query = 0;

//     uint32_t user_ref_num = 1;

//     SoupBinTcp &sbt_;

// public:
//     Builder_OUCH_NASDAQ(SoupBinTcp& sbt) noexcept;

//     Buffer_ONQ *build(Order& order) noexcept;  
   
// private:
//     inline void buildEnterOrder(Order& order, char* buf) noexcept
//     {
//         Endian::write_u32_be(buf + 1, user_ref_num);
//         *(buf + 5) = order.side == Side::Buy ? 'B' : 'S';
//         Endian::write_u32_be(buf + 6, order.quantity);
//         std::memcpy(buf + 10, &order.symbol, order.real_symbol_len);
//         Endian::write_u64_be(buf + 18, static_cast<uint64_t>(order.price));
//         *(buf + 26) = static_cast<uint8_t>(order.time_in_force);
//         std::memcpy(buf + 31, &order.client_order_token, order.real_cl_ord_token_len);

//         order.user_ref_num = user_ref_num++;
    
//     }

//     inline void buildReplaceOrder(Order& order, char* buf) noexcept
//     {
//         Endian::write_u32_be(buf + 1, order.pre_user_ref_num);
//         Endian::write_u32_be(buf + 5, user_ref_num);
//         Endian::write_u32_be(buf + 9, order.quantity);
//         Endian::write_u64_be(buf + 13, static_cast<uint64_t>(order.price));
//         *(buf + 21) = static_cast<uint8_t>(order.time_in_force);      
//         std::memcpy(buf + 24, &order.client_order_token, order.real_cl_ord_token_len);

//         order.pre_user_ref_num = order.user_ref_num;
//         order.user_ref_num = user_ref_num++;
    
//     }

//     inline void buildCancelOrder(Order& order, char* buf) noexcept
//     {
//         Endian::write_u32_be(buf + 1, user_ref_num);
//         Endian::write_u32_be(buf + 5, order.quantity);
//     }

//     inline void buildModifyOrder(Order& order, char* buf) noexcept
//     {
//         Endian::write_u32_be(buf + 1, user_ref_num);
//         *(buf + 5) = order.side == Side::Buy ? 'B' : 'S';
//         Endian::write_u32_be(buf + 6, order.quantity);
//     }

//     using OuchHandlerFunc = void (Builder_OUCH_NASDAQ::*)(Order & order, char* buf) noexcept;
//     static constexpr std::array<OuchHandlerFunc, 4> OuchMessageBuilders =
//     {
//             &Builder_OUCH_NASDAQ::buildEnterOrder,
//             &Builder_OUCH_NASDAQ::buildReplaceOrder,
//             &Builder_OUCH_NASDAQ::buildCancelOrder,
//             &Builder_OUCH_NASDAQ::buildModifyOrder,
//     };

// };
