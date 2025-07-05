#pragma once

#include "BaseParser.h"
#include "GeneratedITCHMessages.h"
#include "Logger.h"
#include <cstdint>
#include <cstring>
#include <array>
#include <string_view>
#include <variant>

using ITCHMessage = std::variant<
    ITCHAddOrderMessage,
    ITCHAddOrderMPIDMessage,
    ITCHCancelMessage,
    ITCHExecutedMessage,
    ITCHExecutedWithPriceMessage,
    ITCHDeleteMessage,
    ITCHTradeMessage,
    ITCHSystemEventMessage>;

class ITCHParser
{
private:
    static constexpr size_t MAX_MESSAGES = 8;

    using MessageHandlerFunc = void (*)(const char *, ITCHMessage &) noexcept;

    static inline std::array<MessageHandlerFunc, MAX_MESSAGES> makeMessageHandlersLookup() noexcept
    {
        alignas(64) std::array<MessageHandlerFunc, MAX_MESSAGES> handlers{};

        handlers[0] = [](const char *data, ITCHMessage &msg) noexcept
        {
            ITCHAddOrderMessage m{};
            std::memcpy(&m.message_type, data, 1);
            std::memcpy(&m.timestamp, data + 1, 8);
            std::memcpy(&m.order_ref, data + 9, 8);
            std::memcpy(&m.side, data + 17, 1);
            std::memcpy(&m.quantity, data + 18, 4);
            std::memcpy(&m.stock, data + 22, 8);
            std::memcpy(&m.price, data + 30, 4);
            msg = m;
        };

        handlers[1] = [](const char *data, ITCHMessage &msg) noexcept
        {
            ITCHAddOrderMPIDMessage m{};
            std::memcpy(&m.message_type, data, 1);
            std::memcpy(&m.timestamp, data + 1, 8);
            std::memcpy(&m.order_ref, data + 9, 8);
            std::memcpy(&m.side, data + 17, 1);
            std::memcpy(&m.quantity, data + 18, 4);
            std::memcpy(&m.stock, data + 22, 8);
            std::memcpy(&m.price, data + 30, 4);
            std::memcpy(&m.mpid, data + 34, 4);
            msg = m;
        };

        handlers[2] = [](const char *data, ITCHMessage &msg) noexcept
        {
            ITCHCancelMessage m{};
            std::memcpy(&m.message_type, data, 1);
            std::memcpy(&m.timestamp, data + 1, 8);
            std::memcpy(&m.order_ref, data + 9, 8);
            std::memcpy(&m.cancelled_quantity, data + 17, 4);
            msg = m;
        };

        handlers[3] = [](const char *data, ITCHMessage &msg) noexcept
        {
            ITCHExecutedMessage m{};
            std::memcpy(&m.message_type, data, 1);
            std::memcpy(&m.timestamp, data + 1, 8);
            std::memcpy(&m.order_ref, data + 9, 8);
            std::memcpy(&m.executed_quantity, data + 17, 4);
            msg = m;
        };

        handlers[4] = [](const char *data, ITCHMessage &msg) noexcept
        {
            ITCHExecutedWithPriceMessage m{};
            std::memcpy(&m.message_type, data, 1);
            std::memcpy(&m.timestamp, data + 1, 8);
            std::memcpy(&m.order_ref, data + 9, 8);
            std::memcpy(&m.executed_quantity, data + 17, 4);
            std::memcpy(&m.execution_price, data + 21, 4);
            msg = m;
        };

        handlers[5] = [](const char *data, ITCHMessage &msg) noexcept
        {
            ITCHDeleteMessage m{};
            std::memcpy(&m.message_type, data, 1);
            std::memcpy(&m.timestamp, data + 1, 8);
            std::memcpy(&m.order_ref, data + 9, 8);
            msg = m;
        };

        handlers[6] = [](const char *data, ITCHMessage &msg) noexcept
        {
            ITCHTradeMessage m{};
            std::memcpy(&m.message_type, data, 1);
            std::memcpy(&m.timestamp, data + 1, 8);
            std::memcpy(&m.order_ref, data + 9, 8);
            std::memcpy(&m.stock, data + 17, 8);
            std::memcpy(&m.quantity, data + 25, 4);
            std::memcpy(&m.price, data + 29, 4);
            std::memcpy(&m.match_id, data + 33, 4);
            msg = m;
        };

        handlers[7] = [](const char *data, ITCHMessage &msg) noexcept
        {
            ITCHSystemEventMessage m{};
            std::memcpy(&m.message_type, data, 1);
            std::memcpy(&m.timestamp, data + 1, 8);
            std::memcpy(&m.event_code, data + 9, 1);
            msg = m;
        };

        return handlers;
    }

    static inline std::array<MessageHandlerFunc, MAX_MESSAGES> MessageHandlers = makeMessageHandlersLookup();

public:
    ITCHMessage parse(const char *data) noexcept
    {
        ITCHMessage ITCHmsg_;
        size_t index = MessageIndex(*data);
        if (index != 99)
            MessageHandlers[index](data, ITCHmsg_);
        return ITCHmsg_;
    }
};
