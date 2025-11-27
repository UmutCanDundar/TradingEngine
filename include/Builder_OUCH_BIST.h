#pragma once

#include "common.h"
#include "Order.h"
#include "Endian.h"
#include "SoupBinTcp.h"

#include <cstdint>
#include <cstddef>
#include <array>
#include <cstring>

/* enum class OuchMessageType : uint8_t
{
    EnterOrder = 0,
    ReplaceOrder = 1,
    CancelOrder = 2,
    CancelByOrderID = 3
}; */

struct ExchangeClientInfo
{
    char client_account[16]; // Client/Account for DM
    char exchange_info[32];  // Account number (first 16 used) for EQM
};

struct alignas(64) Buffer_OBT
{
    static constexpr size_t MAX_MSG_SIZE = 128;

    char msg[MAX_MSG_SIZE];
    uint16_t len;
};

class Builder_OUCH_BIST
{
private:
    static constexpr size_t DAILY_OUCH_BIST_MSG_COUNT = 300'000;
    std::array<Buffer_OBT, DAILY_OUCH_BIST_MSG_COUNT> messages;
    size_t next_buffer_slot = 0;
    ExchangeClientInfo eci{"DMACCOUNT1234", "EXCHANGEINFO1234567890ABCDEFGH"};

    SoupBinTcp &sbt_;    

public:
    Builder_OUCH_BIST(SoupBinTcp &sbt) noexcept : sbt_(sbt) {}

    Buffer_OBT *build(const Order &order) noexcept
    {
        constexpr std::array<size_t, 4> msg_sizes = {114, 122, 15, 14};
        Buffer_OBT *buffer = &messages[next_buffer_slot];
        char *buf = buffer->msg;
        buffer->len = msg_sizes[order.message_type];

        buf = sbt_.WriteSBTHeaderForDataPacket(buf, buffer->len);

        (this->*OuchMessageBuilders[order.message_type])(order, buf);

        return buffer;
    }
   
private:
    inline char* buildEnterOrder(const Order &order, char *buf) noexcept
    {
        *buf = 'O';
        std::memcpy(buf + 1, &order.client_order_token, 14);
        Endian::write_u32_be(buf + 15, order.instrument_id);
        *(buf + 19) = order.side == Side::Buy ? 'B' : 'S';
        Endian::write_u64_be(buf + 20, order.quantity);
        Endian::write_i32_be(buf + 28, order.price);
        *(buf + 32) = static_cast<uint8_t>(order.time_in_force);
        *(buf + 33) = 0;
        std::memcpy(buf + 34, &eci.client_account, 16);
        std::memset(buf + 50, 0, 15);
        std::memcpy(buf + 65, &eci.exchange_info, 32);
        Endian::write_u64_be(buf + 97, 0);
        *(buf + 105) = 1;
        *(buf + 106) = 0;
        *(buf + 107) = 0;
        *(buf + 108) = 0;
        std::memset(buf + 109, 0, 3);
        std::memset(buf + 112, 0, 2);

        return buf;
    }

    inline char* buildReplaceOrder(const Order &order, char *buf) noexcept
    {
        *buf = 'U';
        std::memcpy(buf + 1, &order.fix_org_order_id, 14);
        std::memcpy(buf + 15, &order.client_order_token, 14);
        Endian::write_u64_be(buf + 29, order.quantity);
        Endian::write_i32_be(buf + 37, order.price);
        *(buf + 41) = 0;
        std::memcpy(buf + 42, &eci.client_account, 16);
        std::memset(buf + 58, 0, 15);
        std::memcpy(buf + 73, &eci.exchange_info, 32);
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
    
    using OuchHandlerFunc = char *(Builder_OUCH_BIST::*)(const Order &order, char *buf) noexcept;
    static inline const std::array<OuchHandlerFunc, 4> OuchMessageBuilders = 
    {
        &Builder_OUCH_BIST::buildEnterOrder,
        &Builder_OUCH_BIST::buildReplaceOrder,
        &Builder_OUCH_BIST::buildCancelOrder,
        &Builder_OUCH_BIST::buildCancelByOrderID 
    };
};

