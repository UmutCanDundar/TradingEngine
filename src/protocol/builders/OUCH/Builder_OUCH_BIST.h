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
// - The builder path is largely memory-bound due to sequential buffer writes, multiple memcpy/memset
//   operations, and fixed-layout binary message construction; therefore, forcing inline or constexpr
//   dispatch upfront is not expected to yield meaningful latency improvements.
//   Dispatch is currently implemented via a function table for correctness and clarity. Since the number
//   of message types is small, this can be revisited after profiling: hot paths may be converted to
//   switch-case dispatch to enable inlining and reduce instruction count, provided instruction cache
//   pressure remains acceptable.(*)TESTED - see benchmark report
//
// - Message builders are marked inline for clarity and future flexibility;
//   however, due to function-pointer-based dispatch, they are not expected
//   to inline in the current design.
//  
// - After observing real traffic patterns, message-type–specific prefilled templates may be introduced
//   to further reduce runtime instruction count. In such a design, invariant fields would be written
//   once at initialization time, and only truly dynamic fields (e.g., quantity, price, order IDs)
//   would be patched per message, trading memory for lower instruction and store pressure on hot paths.(*)TESTED - see benchmark report
// ======================================================================================================

//FUNC_PTR PRE-FILL
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

struct Buffer_OBT
{
    char* msg;
    uint16_t len; //SBT HEADER + MSG SIZE
};

struct Buffer_OBT_Enter : Buffer_OBT
{
    static constexpr size_t ENTER_SIZE = 117;

    char data[ENTER_SIZE];
    Buffer_OBT_Enter() : Buffer_OBT{data, ENTER_SIZE}
    { 
        std::memset(data, ' ', ENTER_SIZE);
        data[3] = 'O';
        data[36] = 0;
        // std::memset(data + 53, ' ', 15);
        Endian::write_u64_be(data + 100, 0);
        data[108] = 1;
        data[109] = data[110] = data[111] = 0;
        // std::memset(data + 112, ' ', 3);
        std::memset(data + 115, 0, 2); 
    }
};

struct Buffer_OBT_Replace : Buffer_OBT
{
    static constexpr size_t REPLACE_SIZE = 125;

    char data[REPLACE_SIZE];
    Buffer_OBT_Replace() : Buffer_OBT{data, REPLACE_SIZE}
    { 
        std::memset(data, ' ', REPLACE_SIZE);
        data[3] = 'U';
        data[44] = 0;
        // std::memset(data + 61, ' ', 15); 
        Endian::write_u64_be(data + 108, 0);
        data[116] = 1;
        std::memset(data + 117, 0, 8); 
    }
};

struct Buffer_OBT_Cancel : Buffer_OBT
{
    static constexpr size_t CANCEL_SIZE = 18;

    char data[CANCEL_SIZE];
    Buffer_OBT_Cancel() : Buffer_OBT{data, CANCEL_SIZE}
    { 
        std::memset(data, ' ', CANCEL_SIZE); 
        data[3] = 'X';
    }
};

struct Buffer_OBT_CancelID : Buffer_OBT
{
    static constexpr size_t CANCELID_SIZE = 17;

    char data[CANCELID_SIZE];
    Buffer_OBT_CancelID() : Buffer_OBT{data, CANCELID_SIZE}
    { 
        std::memset(data, ' ', CANCELID_SIZE); 
        data[3] = 'Y';
    }
};

class Builder_OUCH_BIST
{
private:
    static constexpr size_t DAILY_ENTER = 1 << 1;    
    static constexpr size_t DAILY_REPLACE = 1 << 1;  
    static constexpr size_t DAILY_CANCEL = 1 << 1;   
    static constexpr size_t DAILY_CANCELID = 1 << 1; 

    std::array<Buffer_OBT_Enter, DAILY_ENTER> enter_messages;
    std::array<Buffer_OBT_Replace, DAILY_REPLACE> replace_messages;
    std::array<Buffer_OBT_Cancel, DAILY_CANCEL> cancel_messages;
    std::array<Buffer_OBT_CancelID, DAILY_CANCELID> cancelid_messages;
    
    size_t next_enter = 0;
    size_t next_replace = 0;
    size_t next_cancel = 0;
    size_t next_cancelid = 0;

    const std::array<ExchangeClientInfo_local, ACC_COUNT> eci_local_;

    SoupBinTcp &sbt_;

public:
    Builder_OUCH_BIST(SoupBinTcp &sbt, SessionManager &sess_mngr) noexcept;

    Buffer_OBT* build(const Order &order) noexcept;

private:
    std::array<ExchangeClientInfo_local, ACC_COUNT> initialize_exc_info(SessionManager &sess_mngr) noexcept;
    

    inline void buildEnterOrder(const Order &order, char *buf) noexcept
    {
        std::memcpy(buf + 1, &order.client_order_token, order.real_cl_ord_token_len);
        Endian::write_u32_be(buf + 15, order.instrument_id);
        *(buf + 19) = order.side == Side::Buy ? 'B' : 'S';
        Endian::write_u64_be(buf + 20, order.quantity);
        Endian::write_i32_be(buf + 28, order.price);
        *(buf + 32) = static_cast<uint8_t>(order.time_in_force);
        std::memcpy(buf + 34, eci_local_[order.account_index].client_account, 16);
        std::memcpy(buf + 65, eci_local_[order.account_index].exchange_info, 32);
    }

    inline void buildReplaceOrder(const Order &order, char *buf) noexcept
    {
        std::memcpy(buf + 1, &order.fix_org_order_id, 14);
        std::memcpy(buf + 15, &order.client_order_token, order.real_cl_ord_token_len);
        Endian::write_u64_be(buf + 29, order.quantity);
        Endian::write_i32_be(buf + 37, order.price);
        std::memcpy(buf + 42, eci_local_[order.account_index].client_account, 16);
        std::memcpy(buf + 73, eci_local_[order.account_index].exchange_info, 32);
        Endian::write_u64_be(buf + 105, 0);
        *(buf + 113) = 1;
    }

    inline void buildCancelOrder(const Order &order, char *buf) noexcept
    {
        std::memcpy(buf + 1, &order.client_order_token, 14);      
    }

    inline void buildCancelByOrderID(const Order &order, char *buf) noexcept
    {
        Endian::write_u32_be(buf + 1, order.instrument_id);
        *(buf + 5) = order.side == Side::Buy ? 'B' : 'S';
        Endian::write_u64_be(buf + 6, order.order_id);
    }
    
    using OuchHandlerFunc = void (Builder_OUCH_BIST::*)(const Order &order, char *buf) noexcept;
    static constexpr std::array<OuchHandlerFunc, 4> OuchMessageBuilders =
    {
            &Builder_OUCH_BIST::buildEnterOrder,
            &Builder_OUCH_BIST::buildReplaceOrder,
            &Builder_OUCH_BIST::buildCancelOrder,
            &Builder_OUCH_BIST::buildCancelByOrderID
    };
};





//FUNC-PTR NO-PRE-FILL

// #pragma once

// #include "Order.h"
// #include "Endian.h"
// #include "SessionManager.h"

// #include <cstdint>
// #include <cstddef>
// #include <array>
// #include <cstring>
// #include <string_view>

// class SoupBinTcp;

// struct ExchangeClientInfo_local 
// {
//     char client_account[16]; // Client/Account for DM
//     char exchange_info[32];  // Account number (first 16 used) for EQM

//     ExchangeClientInfo_local() = default;

//     ExchangeClientInfo_local(const std::string_view cl_acc, const std::string_view exc_info) 
//     {
//         std::memset(client_account, ' ', 16);
//         std::memset(exchange_info, ' ', 32);
        
//         std::memcpy(client_account, cl_acc.data(), std::min(cl_acc.size(), size_t(16)));
//         std::memcpy(exchange_info, exc_info.data(), std::min(exc_info.size(), size_t(32)));
//     }
// };


// struct Buffer_OBT
// {
//     char msg[126];
//     uint16_t len;
//     Buffer_OBT() { std::memset(msg, ' ', 126); }
// };

// class Builder_OUCH_BIST
// {
// private:
//     static constexpr size_t DAILY = 1 << 17;    // 32'768
    

//     std::array<Buffer_OBT, DAILY>  messages;
    
    
//     size_t next = 0;
  

//     const std::array<ExchangeClientInfo_local, ACC_COUNT> eci_local_;

//     SoupBinTcp &sbt_;

// public:
//     Builder_OUCH_BIST(SoupBinTcp &sbt, SessionManager &sess_mngr) noexcept;

//     Buffer_OBT* build(const Order &order) noexcept;

// private:
//     std::array<ExchangeClientInfo_local, ACC_COUNT> initialize_exc_info(SessionManager &sess_mngr) noexcept;
    

//     inline void buildEnterOrder(const Order &order, char *buf) noexcept
//     {
//         std::memcpy(buf + 1, &order.client_order_token, order.real_cl_ord_token_len);
//         Endian::write_u32_be(buf + 15, order.instrument_id);
//         *(buf + 19) = order.side == Side::Buy ? 'B' : 'S';
//         Endian::write_u64_be(buf + 20, order.quantity);
//         Endian::write_i32_be(buf + 28, order.price);
//         *(buf + 32) = static_cast<uint8_t>(order.time_in_force);
//         std::memcpy(buf + 34, eci_local_[order.account_index].client_account, 16);
//         std::memcpy(buf + 65, eci_local_[order.account_index].exchange_info, 32);
//          buf[0] = 'O';
//         buf[33] = 0;
//         // std::memset(buf + 50, ' ', 15);
//         Endian::write_u64_be(buf + 97, 0);
//         buf[105] = 1;
//         buf[106] = buf[107] = buf[108] = 0;
//         // std::memset(buf + 109, ' ', 3);
//         std::memset(buf + 112, 0, 2);
//     }

//     inline void buildReplaceOrder(const Order &order, char *buf) noexcept
//     {
//         std::memcpy(buf + 1, &order.fix_org_order_id, 14);
//         std::memcpy(buf + 15, &order.client_order_token, order.real_cl_ord_token_len);
//         Endian::write_u64_be(buf + 29, order.quantity);
//         Endian::write_i32_be(buf + 37, order.price);
//         std::memcpy(buf + 42, eci_local_[order.account_index].client_account, 16);
//         std::memcpy(buf + 73, eci_local_[order.account_index].exchange_info, 32);
//         Endian::write_u64_be(buf + 105, 0);
//         *(buf + 113) = 1;
//     }

//     inline void buildCancelOrder(const Order &order, char *buf) noexcept
//     {
//         std::memcpy(buf + 1, &order.client_order_token, 14);
//     }

//     inline void buildCancelByOrderID(const Order &order, char *buf) noexcept
//     {
//         Endian::write_u32_be(buf + 1, order.instrument_id);
//         *(buf + 5) = order.side == Side::Buy ? 'B' : 'S';
//         Endian::write_u64_be(buf + 6, order.order_id);
//     }
    
//     using OuchHandlerFunc = void (Builder_OUCH_BIST::*)(const Order &order, char *buf) noexcept;
//     static constexpr std::array<OuchHandlerFunc, 4> OuchMessageBuilders =
//     {
//             &Builder_OUCH_BIST::buildEnterOrder,
//             &Builder_OUCH_BIST::buildReplaceOrder,
//             &Builder_OUCH_BIST::buildCancelOrder,
//             &Builder_OUCH_BIST::buildCancelByOrderID
//     };
// };







// SWITCH-CASE PRE-FILL

// #pragma once

// #include "Order.h"
// #include "Endian.h"
// #include "SessionManager.h"

// #include <cstdint>
// #include <cstddef>
// #include <array>
// #include <cstring>
// #include <string_view>

// class SoupBinTcp;

// struct ExchangeClientInfo_local 
// {
//     char client_account[16]; // Client/Account for DM
//     char exchange_info[32];  // Account number (first 16 used) for EQM

//     ExchangeClientInfo_local() = default;

//     ExchangeClientInfo_local(const std::string_view cl_acc, const std::string_view exc_info) 
//     {
//         std::memset(client_account, ' ', 16);
//         std::memset(exchange_info, ' ', 32);
        
//         std::memcpy(client_account, cl_acc.data(), std::min(cl_acc.size(), size_t(16)));
//         std::memcpy(exchange_info, exc_info.data(), std::min(exc_info.size(), size_t(32)));
//     }
// };

// struct Buffer_OBT_Enter   { char msg[117]; uint16_t len = 117; Buffer_OBT_Enter() { std::memset(msg, ' ', 117); }}; 
// struct Buffer_OBT_Replace { char msg[125]; uint16_t len = 125; Buffer_OBT_Replace() { std::memset(msg, ' ', 125); }};
// struct Buffer_OBT_Cancel  { char msg[18];  uint16_t len = 18; Buffer_OBT_Cancel() { std::memset(msg, ' ', 18); }};
// struct Buffer_OBT_CancelID{ char msg[17];  uint16_t len = 17; Buffer_OBT_CancelID() { std::memset(msg, ' ', 17); }};

// class Builder_OUCH_BIST
// {
// private:
//     static constexpr size_t DAILY_ENTER = 1 << 15;    // 32'768
//     static constexpr size_t DAILY_REPLACE = 1 << 14;  // 16'384
//     static constexpr size_t DAILY_CANCEL = 1 << 13;   // 8'192
//     static constexpr size_t DAILY_CANCELID = 1 << 13; // 8'192

//     std::array<Buffer_OBT_Enter, DAILY_ENTER> enter_messages;
//     std::array<Buffer_OBT_Replace, DAILY_REPLACE> replace_messages;
//     std::array<Buffer_OBT_Cancel, DAILY_CANCEL> cancel_messages;
//     std::array<Buffer_OBT_CancelID, DAILY_CANCELID> cancelid_messages;
    
//     size_t next_enter = 0;
//     size_t next_replace = 0;
//     size_t next_cancel = 0;
//     size_t next_cancelid = 0;

//     const std::array<ExchangeClientInfo_local, ACC_COUNT> eci_local_;

//     SoupBinTcp &sbt_;

// public:
//     Builder_OUCH_BIST(SoupBinTcp &sbt, SessionManager &sess_mngr) noexcept;

//     Buffer_OBT_Enter* build_enter(const Order& order) noexcept;
//     Buffer_OBT_Replace* build_replace(const Order& order) noexcept;
//     Buffer_OBT_Cancel* build_cancel(const Order& order) noexcept;
//     Buffer_OBT_CancelID* build_cancelid(const Order& order) noexcept;
   
// private:
//     std::array<ExchangeClientInfo_local, ACC_COUNT> initialize_exc_info(SessionManager &sess_mngr) noexcept;
    

//     __attribute__((always_inline)) inline char* buildEnterOrder(const Order &order, char *buf) noexcept
//     {
//         std::memcpy(buf + 1, &order.client_order_token, order.real_cl_ord_token_len);
//         Endian::write_u32_be(buf + 15, order.instrument_id);
//         *(buf + 19) = order.side == Side::Buy ? 'B' : 'S';
//         Endian::write_u64_be(buf + 20, order.quantity);
//         Endian::write_i32_be(buf + 28, order.price);
//         *(buf + 32) = static_cast<uint8_t>(order.time_in_force);
//         std::memcpy(buf + 34, eci_local_[order.account_index].client_account, 16);
//         std::memcpy(buf + 65, eci_local_[order.account_index].exchange_info, 32);

//         return buf;
//     }

//     __attribute__((always_inline)) inline char* buildReplaceOrder(const Order &order, char *buf) noexcept
//     {
//         std::memcpy(buf + 1, &order.fix_org_order_id, 14);
//         std::memcpy(buf + 15, &order.client_order_token, order.real_cl_ord_token_len);
//         Endian::write_u64_be(buf + 29, order.quantity);
//         Endian::write_i32_be(buf + 37, order.price);
//         std::memcpy(buf + 42, eci_local_[order.account_index].client_account, 16);
//         std::memcpy(buf + 73, eci_local_[order.account_index].exchange_info, 32);
//         Endian::write_u64_be(buf + 105, 0);
//         *(buf + 113) = 1;

//         return buf;
//     }

//     __attribute__((always_inline)) inline char* buildCancelOrder(const Order &order, char *buf) noexcept
//     {
//         std::memcpy(buf + 1, &order.client_order_token, 14);
      
//         return buf;
//     }

//     __attribute__((always_inline)) inline char* buildCancelByOrderID(const Order &order, char *buf) noexcept
//     {
//         Endian::write_u32_be(buf + 1, order.instrument_id);
//         *(buf + 5) = order.side == Side::Buy ? 'B' : 'S';
//         Endian::write_u64_be(buf + 6, order.order_id);

//         return buf;
//     }
    
// };
































// #pragma once

// #include "Order.h"
// #include "Endian.h"
// #include "SessionManager.h"

// #include <cstdint>
// #include <cstddef>
// #include <array>
// #include <cstring>
// #include <string_view>

// class SoupBinTcp;

// struct ExchangeClientInfo_local 
// {
//     char client_account[16]; // Client/Account for DM
//     char exchange_info[32];  // Account number (first 16 used) for EQM

//     ExchangeClientInfo_local() = default;

//     ExchangeClientInfo_local(const std::string_view cl_acc, const std::string_view exc_info) 
//     {
//         std::memset(client_account, ' ', 16);
//         std::memset(exchange_info, ' ', 32);
        
//         std::memcpy(client_account, cl_acc.data(), std::min(cl_acc.size(), size_t(16)));
//         std::memcpy(exchange_info, exc_info.data(), std::min(exc_info.size(), size_t(32)));
//     }
// };

// struct Buffer_OBT // May be separated per msg type after observing real traffic to avoid unnecessary set to some fields
// {
//     static constexpr size_t MAX_MSG_SIZE = 126;

//     char msg[MAX_MSG_SIZE];
//     uint16_t len;

//    Buffer_OBT() { std::memset(msg, ' ', MAX_MSG_SIZE); }
// };

// class Builder_OUCH_BIST
// {
// private:
//     static constexpr size_t DAILY_ENTER = 1 << 15;    // 32'768
//     static constexpr size_t DAILY_REPLACE = 1 << 14;  // 16'384
//     static constexpr size_t DAILY_CANCEL = 1 << 13;   // 8'192
//     static constexpr size_t DAILY_CANCELID = 1 << 13; // 8'192

//     static constexpr size_t replace_offset = DAILY_ENTER;
//     static constexpr size_t cancel_offset = replace_offset + DAILY_REPLACE;
//     static constexpr size_t cancelid_offset = cancel_offset + DAILY_CANCEL;

//     static constexpr size_t DAILY_OUCH_BIST_MSG_COUNT = 1 << 16;
//     std::array<Buffer_OBT, DAILY_OUCH_BIST_MSG_COUNT> messages;
    
//     size_t next_enter = 0;
//     size_t next_replace = DAILY_ENTER;
//     size_t next_cancel = DAILY_REPLACE;
//     size_t next_cancelid = DAILY_CANCEL;

//     const std::array<ExchangeClientInfo_local, ACC_COUNT> eci_local_;

//     SoupBinTcp &sbt_;

// public:
//     Builder_OUCH_BIST(SoupBinTcp &sbt, SessionManager &sess_mngr) noexcept;

//     Buffer_OBT* build(const Order &order) noexcept;
   
// private:
//     std::array<ExchangeClientInfo_local, ACC_COUNT> initialize_exc_info(SessionManager &sess_mngr) noexcept;
    

//     inline char* buildEnterOrder(const Order &order, char *buf) noexcept
//     {
//         std::memcpy(buf + 1, &order.client_order_token, order.real_cl_ord_token_len);
//         Endian::write_u32_be(buf + 15, order.instrument_id);
//         *(buf + 19) = order.side == Side::Buy ? 'B' : 'S';
//         Endian::write_u64_be(buf + 20, order.quantity);
//         Endian::write_i32_be(buf + 28, order.price);
//         *(buf + 32) = static_cast<uint8_t>(order.time_in_force);
//         std::memcpy(buf + 34, eci_local_[order.account_index].client_account, 16);
//         std::memcpy(buf + 65, eci_local_[order.account_index].exchange_info, 32);

//         return buf;
//     }

//     inline char* buildReplaceOrder(const Order &order, char *buf) noexcept
//     {
//         std::memcpy(buf + 1, &order.fix_org_order_id, 14);
//         std::memcpy(buf + 15, &order.client_order_token, order.real_cl_ord_token_len);
//         Endian::write_u64_be(buf + 29, order.quantity);
//         Endian::write_i32_be(buf + 37, order.price);
//         std::memcpy(buf + 42, eci_local_[order.account_index].client_account, 16);
//         std::memcpy(buf + 73, eci_local_[order.account_index].exchange_info, 32);
//         Endian::write_u64_be(buf + 105, 0);
//         *(buf + 113) = 1;

//         return buf;
//     }

//     inline char* buildCancelOrder(const Order &order, char *buf) noexcept
//     {
//         std::memcpy(buf + 1, &order.client_order_token, 14);
      
//         return buf;
//     }

//     inline char* buildCancelByOrderID(const Order &order, char *buf) noexcept
//     {
//         Endian::write_u32_be(buf + 1, order.instrument_id);
//         *(buf + 5) = order.side == Side::Buy ? 'B' : 'S';
//         Endian::write_u64_be(buf + 6, order.order_id);

//         return buf;
//     }
    
//     using OuchHandlerFunc = char* (Builder_OUCH_BIST::*)(const Order &order, char *buf) noexcept;
//     static constexpr std::array<OuchHandlerFunc, 4> OuchMessageBuilders =
//     {
//             &Builder_OUCH_BIST::buildEnterOrder,
//             &Builder_OUCH_BIST::buildReplaceOrder,
//             &Builder_OUCH_BIST::buildCancelOrder,
//             &Builder_OUCH_BIST::buildCancelByOrderID
//     };
// };




// #pragma once

// #include "Order.h"
// #include "Endian.h"
// #include "SessionManager.h"

// #include <cstdint>
// #include <cstddef>
// #include <array>
// #include <cstring>
// #include <string_view>

// class SoupBinTcp;

// struct ExchangeClientInfo_local 
// {
//     char client_account[16]; // Client/Account for DM
//     char exchange_info[32];  // Account number (first 16 used) for EQM

//     ExchangeClientInfo_local() = default;

//     ExchangeClientInfo_local(const std::string_view cl_acc, const std::string_view exc_info) 
//     {
//         std::memset(client_account, ' ', 16);
//         std::memset(exchange_info, ' ', 32);
        
//         std::memcpy(client_account, cl_acc.data(), std::min(cl_acc.size(), size_t(16)));
//         std::memcpy(exchange_info, exc_info.data(), std::min(exc_info.size(), size_t(32)));
//     }
// };

// struct Buffer_OBT // May be separated per msg type after observing real traffic to avoid unnecessary set to some fields
// {
//     static constexpr size_t MAX_MSG_SIZE = 128;

//     char msg[MAX_MSG_SIZE];
//     uint16_t len;

//    Buffer_OBT() { std::memset(msg, ' ', MAX_MSG_SIZE); }
// };

// class Builder_OUCH_BIST
// {
// private:
//     static constexpr size_t DAILY_OUCH_BIST_MSG_COUNT = 65'536;
//     std::array<Buffer_OBT, DAILY_OUCH_BIST_MSG_COUNT> messages;
//     size_t next_buffer_slot = 0;

//     const std::array<ExchangeClientInfo_local, ACC_COUNT> eci_local_;

//     SoupBinTcp &sbt_;

// public:
//     Builder_OUCH_BIST(SoupBinTcp &sbt, SessionManager &sess_mngr) noexcept;

//     Buffer_OBT *build(const Order &order) noexcept;
   
// private:
//     std::array<ExchangeClientInfo_local, ACC_COUNT> initialize_exc_info(SessionManager &sess_mngr) noexcept;
    

//     inline char* buildEnterOrder(const Order &order, char *buf) noexcept
//     {
//         *buf = 'O';
//         std::memcpy(buf + 1, &order.client_order_token, order.real_cl_ord_token_len);
//         Endian::write_u32_be(buf + 15, order.instrument_id);
//         *(buf + 19) = order.side == Side::Buy ? 'B' : 'S';
//         Endian::write_u64_be(buf + 20, order.quantity);
//         Endian::write_i32_be(buf + 28, order.price);
//         *(buf + 32) = static_cast<uint8_t>(order.time_in_force);
//         *(buf + 33) = 0;
//         std::memcpy(buf + 34, eci_local_[order.account_index].client_account, 16);
//         // std::memset(buf + 50, ' ', 15);
//         std::memcpy(buf + 65, eci_local_[order.account_index].exchange_info, 32);
//         Endian::write_u64_be(buf + 97, 0);
//         *(buf + 105) = 1;
//         *(buf + 106) = 0;
//         *(buf + 107) = 0;
//         *(buf + 108) = 0;
//         std::memset(buf + 109, ' ', 3);
//         std::memset(buf + 112, 0, 2);

//         return buf;
//     }

//     inline char* buildReplaceOrder(const Order &order, char *buf) noexcept
//     {
//         *buf = 'U';
//         std::memcpy(buf + 1, &order.fix_org_order_id, 14);
//         std::memcpy(buf + 15, &order.client_order_token, order.real_cl_ord_token_len);
//         Endian::write_u64_be(buf + 29, order.quantity);
//         Endian::write_i32_be(buf + 37, order.price);
//         *(buf + 41) = 0;
//         std::memcpy(buf + 42, eci_local_[order.account_index].client_account, 16);
//         // std::memset(buf + 58, ' ', 15);
//         std::memcpy(buf + 73, eci_local_[order.account_index].exchange_info, 32);
//         Endian::write_u64_be(buf + 105, 0);
//         *(buf + 113) = 1;
//         std::memset(buf + 114, 0, 8);

//         return buf;
//     }

//     inline char* buildCancelOrder(const Order &order, char *buf) noexcept
//     {
//         *buf = 'X';
//         std::memcpy(buf + 1, &order.client_order_token, 14);
      
//         return buf;
//     }

//     inline char* buildCancelByOrderID(const Order &order, char *buf) noexcept
//     {
//         *buf = 'Y';
//         Endian::write_u32_be(buf + 1, order.instrument_id);
//         *(buf + 5) = order.side == Side::Buy ? 'B' : 'S';
//         Endian::write_u64_be(buf + 6, order.order_id);

//         return buf;
//     }
    
//     using OuchHandlerFunc = char* (Builder_OUCH_BIST::*)(const Order &order, char *buf) noexcept;
//     static constexpr std::array<OuchHandlerFunc, 4> OuchMessageBuilders =
//     {
//             &Builder_OUCH_BIST::buildEnterOrder,
//             &Builder_OUCH_BIST::buildReplaceOrder,
//             &Builder_OUCH_BIST::buildCancelOrder,
//             &Builder_OUCH_BIST::buildCancelByOrderID
//     };
// };



// #pragma once

// #include "Order.h"
// #include "Endian.h"
// #include "SessionManager.h"

// #include <cstdint>
// #include <cstddef>
// #include <array>
// #include <cstring>
// #include <string_view>

// class SoupBinTcp;

// struct ExchangeClientInfo_local 
// {
//     char client_account[16]; // Client/Account for DM
//     char exchange_info[32];  // Account number (first 16 used) for EQM

//     ExchangeClientInfo_local() = default;

//     ExchangeClientInfo_local(const std::string_view cl_acc, const std::string_view exc_info) 
//     {
//         std::memset(client_account, ' ', 16);
//         std::memset(exchange_info, ' ', 32);
        
//         std::memcpy(client_account, cl_acc.data(), std::min(cl_acc.size(), size_t(16)));
//         std::memcpy(exchange_info, exc_info.data(), std::min(exc_info.size(), size_t(32)));
//     }
// };

// struct Buffer_OBT // May be separated per msg type after observing real traffic to avoid unnecessary set to some fields
// {
//     static constexpr size_t MAX_MSG_SIZE = 126;

//     char msg[MAX_MSG_SIZE];
//     uint16_t len;

//    Buffer_OBT() { std::memset(msg, ' ', MAX_MSG_SIZE); }
// };

// class Builder_OUCH_BIST
// {
// private:
//     static constexpr size_t DAILY_ENTER = 1 << 15;    // 32'768
//     static constexpr size_t DAILY_REPLACE = 1 << 14;  // 16'384
//     static constexpr size_t DAILY_CANCEL = 1 << 13;   // 8'192
//     static constexpr size_t DAILY_CANCELID = 1 << 13; // 8'192

//     static constexpr size_t replace_offset = DAILY_ENTER;
//     static constexpr size_t cancel_offset = replace_offset + DAILY_REPLACE;
//     static constexpr size_t cancelid_offset = cancel_offset + DAILY_CANCEL;

//     static constexpr size_t DAILY_OUCH_BIST_MSG_COUNT = 1 << 16;
//     std::array<Buffer_OBT, DAILY_OUCH_BIST_MSG_COUNT> messages;
    
//     size_t next_enter = 0;
//     size_t next_replace = DAILY_ENTER;
//     size_t next_cancel = DAILY_REPLACE;
//     size_t next_cancelid = DAILY_CANCEL;

//     const std::array<ExchangeClientInfo_local, ACC_COUNT> eci_local_;

//     SoupBinTcp &sbt_;

// public:
//     Builder_OUCH_BIST(SoupBinTcp &sbt, SessionManager &sess_mngr) noexcept;

//     Buffer_OBT* build(const Order &order) noexcept;
   
// private:
//     std::array<ExchangeClientInfo_local, ACC_COUNT> initialize_exc_info(SessionManager &sess_mngr) noexcept;
    

//     inline char* buildEnterOrder(const Order &order, char *buf) noexcept
//     {
//         std::memcpy(buf + 1, &order.client_order_token, order.real_cl_ord_token_len);
//         Endian::write_u32_be(buf + 15, order.instrument_id);
//         *(buf + 19) = order.side == Side::Buy ? 'B' : 'S';
//         Endian::write_u64_be(buf + 20, order.quantity);
//         Endian::write_i32_be(buf + 28, order.price);
//         *(buf + 32) = static_cast<uint8_t>(order.time_in_force);
//         std::memcpy(buf + 34, eci_local_[order.account_index].client_account, 16);
//         std::memcpy(buf + 65, eci_local_[order.account_index].exchange_info, 32);

//         return buf;
//     }

//     inline char* buildReplaceOrder(const Order &order, char *buf) noexcept
//     {
//         std::memcpy(buf + 1, &order.fix_org_order_id, 14);
//         std::memcpy(buf + 15, &order.client_order_token, order.real_cl_ord_token_len);
//         Endian::write_u64_be(buf + 29, order.quantity);
//         Endian::write_i32_be(buf + 37, order.price);
//         std::memcpy(buf + 42, eci_local_[order.account_index].client_account, 16);
//         std::memcpy(buf + 73, eci_local_[order.account_index].exchange_info, 32);
//         Endian::write_u64_be(buf + 105, 0);
//         *(buf + 113) = 1;

//         return buf;
//     }

//     inline char* buildCancelOrder(const Order &order, char *buf) noexcept
//     {
//         std::memcpy(buf + 1, &order.client_order_token, 14);
      
//         return buf;
//     }

//     inline char* buildCancelByOrderID(const Order &order, char *buf) noexcept
//     {
//         Endian::write_u32_be(buf + 1, order.instrument_id);
//         *(buf + 5) = order.side == Side::Buy ? 'B' : 'S';
//         Endian::write_u64_be(buf + 6, order.order_id);

//         return buf;
//     }
    
//     using OuchHandlerFunc = char* (Builder_OUCH_BIST::*)(const Order &order, char *buf) noexcept;
//     static constexpr std::array<OuchHandlerFunc, 4> OuchMessageBuilders =
//     {
//             &Builder_OUCH_BIST::buildEnterOrder,
//             &Builder_OUCH_BIST::buildReplaceOrder,
//             &Builder_OUCH_BIST::buildCancelOrder,
//             &Builder_OUCH_BIST::buildCancelByOrderID
//     };
// };




// #pragma once

// #include "Order.h"
// #include "Endian.h"
// #include "SessionManager.h"

// #include <cstdint>
// #include <cstddef>
// #include <array>
// #include <cstring>
// #include <string_view>

// class SoupBinTcp;

// struct ExchangeClientInfo_local 
// {
//     char client_account[16]; // Client/Account for DM
//     char exchange_info[32];  // Account number (first 16 used) for EQM

//     ExchangeClientInfo_local() = default;

//     ExchangeClientInfo_local(const std::string_view cl_acc, const std::string_view exc_info) 
//     {
//         std::memset(client_account, ' ', 16);
//         std::memset(exchange_info, ' ', 32);
        
//         std::memcpy(client_account, cl_acc.data(), std::min(cl_acc.size(), size_t(16)));
//         std::memcpy(exchange_info, exc_info.data(), std::min(exc_info.size(), size_t(32)));
//     }
// };

// struct Buffer_OBT // May be separated per msg type after observing real traffic to avoid unnecessary set to some fields
// {
//     static constexpr size_t MAX_MSG_SIZE = 128;

//     char msg[MAX_MSG_SIZE];
//     uint16_t len;

//    Buffer_OBT() { std::memset(msg, ' ', MAX_MSG_SIZE); }
// };

// class Builder_OUCH_BIST
// {
// private:
//     static constexpr size_t DAILY_OUCH_BIST_MSG_COUNT = 65'536;
//     std::array<Buffer_OBT, DAILY_OUCH_BIST_MSG_COUNT> messages;
//     size_t next_buffer_slot = 0;

//     const std::array<ExchangeClientInfo_local, ACC_COUNT> eci_local_;

//     SoupBinTcp &sbt_;

// public:
//     Builder_OUCH_BIST(SoupBinTcp &sbt, SessionManager &sess_mngr) noexcept;

//     Buffer_OBT *build(const Order &order) noexcept;
   
// private:
//     std::array<ExchangeClientInfo_local, ACC_COUNT> initialize_exc_info(SessionManager &sess_mngr) noexcept;
    

//     inline char* buildEnterOrder(const Order &order, char *buf) noexcept
//     {
//         *buf = 'O';
//         std::memcpy(buf + 1, &order.client_order_token, order.real_cl_ord_token_len);
//         Endian::write_u32_be(buf + 15, order.instrument_id);
//         *(buf + 19) = order.side == Side::Buy ? 'B' : 'S';
//         Endian::write_u64_be(buf + 20, order.quantity);
//         Endian::write_i32_be(buf + 28, order.price);
//         *(buf + 32) = static_cast<uint8_t>(order.time_in_force);
//         *(buf + 33) = 0;
//         std::memcpy(buf + 34, eci_local_[order.account_index].client_account, 16);
//         // std::memset(buf + 50, ' ', 15);
//         std::memcpy(buf + 65, eci_local_[order.account_index].exchange_info, 32);
//         Endian::write_u64_be(buf + 97, 0);
//         *(buf + 105) = 1;
//         *(buf + 106) = 0;
//         *(buf + 107) = 0;
//         *(buf + 108) = 0;
//         std::memset(buf + 109, ' ', 3);
//         std::memset(buf + 112, 0, 2);

//         return buf;
//     }

//     inline char* buildReplaceOrder(const Order &order, char *buf) noexcept
//     {
//         *buf = 'U';
//         std::memcpy(buf + 1, &order.fix_org_order_id, 14);
//         std::memcpy(buf + 15, &order.client_order_token, order.real_cl_ord_token_len);
//         Endian::write_u64_be(buf + 29, order.quantity);
//         Endian::write_i32_be(buf + 37, order.price);
//         *(buf + 41) = 0;
//         std::memcpy(buf + 42, eci_local_[order.account_index].client_account, 16);
//         // std::memset(buf + 58, ' ', 15);
//         std::memcpy(buf + 73, eci_local_[order.account_index].exchange_info, 32);
//         Endian::write_u64_be(buf + 105, 0);
//         *(buf + 113) = 1;
//         std::memset(buf + 114, 0, 8);

//         return buf;
//     }

//     inline char* buildCancelOrder(const Order &order, char *buf) noexcept
//     {
//         *buf = 'X';
//         std::memcpy(buf + 1, &order.client_order_token, 14);
      
//         return buf;
//     }

//     inline char* buildCancelByOrderID(const Order &order, char *buf) noexcept
//     {
//         *buf = 'Y';
//         Endian::write_u32_be(buf + 1, order.instrument_id);
//         *(buf + 5) = order.side == Side::Buy ? 'B' : 'S';
//         Endian::write_u64_be(buf + 6, order.order_id);

//         return buf;
//     }
    
//     using OuchHandlerFunc = char* (Builder_OUCH_BIST::*)(const Order &order, char *buf) noexcept;
//     static constexpr std::array<OuchHandlerFunc, 4> OuchMessageBuilders =
//     {
//             &Builder_OUCH_BIST::buildEnterOrder,
//             &Builder_OUCH_BIST::buildReplaceOrder,
//             &Builder_OUCH_BIST::buildCancelOrder,
//             &Builder_OUCH_BIST::buildCancelByOrderID
//     };
// };