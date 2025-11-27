#pragma once

#include "common.h"
#include "Order.h"
#include "Endian.h"
#include "SoupBinTcp.h"

#include <cstdint>
#include <cstddef>
#include <array>
#include <cstring>


struct alignas(64) Buffer_ONQ
{
    static constexpr size_t MAX_MSG_SIZE = 128;

    char msg[MAX_MSG_SIZE];
    uint16_t len;
};

class Builder_OUCH_NASDAQ
{
private:
    static constexpr size_t DAILY_OUCH_NASDAQ_MSG_COUNT = 300'000;
    std::array<Buffer_ONQ, DAILY_OUCH_NASDAQ_MSG_COUNT> messages;
    size_t next_buffer_slot = 0;
    uint32_t user_ref_num = 1;

    SoupBinTcp &sbt_;

public:
    Builder_OUCH_NASDAQ(SoupBinTcp &sbt) noexcept : sbt_(sbt) {}

    Buffer_ONQ *build(Order &order) noexcept
    {
        constexpr std::array<size_t, 5> msg_sizes = {47, 40, 11, 12, 3};
        Buffer_ONQ *buffer = &messages[next_buffer_slot];

        char *buf = buffer->msg;
        buffer->len = msg_sizes[order.message_type];
        buf = sbt_.WriteSBTHeaderForDataPacket(buf, buffer->len);
        (this->*OuchMessageBuilders[order.message_type])(order, buf);
       
        return buffer;
    }
   
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

    inline char *buildAccountQueryRequest(Order &order, char *buf) noexcept  // order is left in arg list on purpose
    {
        *buf = 'Q';
        Endian::write_u16_be(buf + 1, 0);

        return buf;
    }

    using OuchHandlerFunc = char *(Builder_OUCH_NASDAQ::*)(Order &order, char *buf) noexcept;
    static inline const std::array<OuchHandlerFunc, 5> OuchMessageBuilders =
        {
            &Builder_OUCH_NASDAQ::buildEnterOrder,
            &Builder_OUCH_NASDAQ::buildReplaceOrder,
            &Builder_OUCH_NASDAQ::buildCancelOrder,
            &Builder_OUCH_NASDAQ::buildModifyOrder,
            &Builder_OUCH_NASDAQ::buildAccountQueryRequest,
        };
};

