#pragma once

#include "BaseParser.h"
#include "GeneratedITCHMessages.h"
#include <cstdint>
#include <cstring>
#include <array>

union alignas(64) ITCHMessage
{
    ITCHAddOrderMessage addorder;
    ITCHAddOrderMPIDMessage addorderMPID;
    ITCHCancelMessage cancel;
    ITCHExecutedMessage executed;
    ITCHExecutedWithPriceMessage executedwp;
    ITCHDeleteMessage del;
    ITCHTradeMessage trade;
    ITCHSystemEventMessage systemevent;
};

class ITCHParser : public BaseParser<ITCHParser>
{
private:
    static constexpr size_t MAX_MESSAGES = 8;
    ITCHMessage ITCHmsg_{};

    using MessageHandlerFunc = void (*)(const char *, ITCHMessage &) noexcept;

    static inline std::array<MessageHandlerFunc, MAX_MESSAGES> makeMessageHandlersLookup() noexcept
    {
        alignas(64) std::array<MessageHandlerFunc, MAX_MESSAGES> handlers{};

        handlers[0] = [](const char *data, ITCHMessage &msg) noexcept
        {
            std::memcpy(&msg.addorder.message_type, data, 1);
            std::memcpy(&msg.addorder.timestamp, data + 1, 8);
            std::memcpy(&msg.addorder.order_ref, data + 9, 8);
            std::memcpy(&msg.addorder.side, data + 17, 1);
            std::memcpy(&msg.addorder.quantity, data + 18, 4);
            std::memcpy(&msg.addorder.stock, data + 22, 8);
            std::memcpy(&msg.addorder.price, data + 30, 4);
        };

        handlers[1] = [](const char *data, ITCHMessage &msg) noexcept
        {
            std::memcpy(&msg.addorderMPID.message_type, data, 1);
            std::memcpy(&msg.addorderMPID.timestamp, data + 1, 8);
            std::memcpy(&msg.addorderMPID.order_ref, data + 9, 8);
            std::memcpy(&msg.addorderMPID.side, data + 17, 1);
            std::memcpy(&msg.addorderMPID.quantity, data + 18, 4);
            std::memcpy(&msg.addorderMPID.stock, data + 22, 8);
            std::memcpy(&msg.addorderMPID.price, data + 30, 4);
            std::memcpy(&msg.addorderMPID.mpid, data + 34, 4);
        };

        handlers[2] = [](const char *data, ITCHMessage &msg) noexcept
        {
            std::memcpy(&msg.cancel.message_type, data, 1);
            std::memcpy(&msg.cancel.timestamp, data + 1, 8);
            std::memcpy(&msg.cancel.order_ref, data + 9, 8);
            std::memcpy(&msg.cancel.cancelled_quantity, data + 17, 4);
        };

        handlers[3] = [](const char *data, ITCHMessage &msg) noexcept
        {
            std::memcpy(&msg.executed.message_type, data, 1);
            std::memcpy(&msg.executed.timestamp, data + 1, 8);
            std::memcpy(&msg.executed.order_ref, data + 9, 8);
            std::memcpy(&msg.executed.executed_quantity, data + 17, 4);
        };

        handlers[4] = [](const char *data, ITCHMessage &msg) noexcept
        {
            std::memcpy(&msg.executedwp.message_type, data, 1);
            std::memcpy(&msg.executedwp.timestamp, data + 1, 8);
            std::memcpy(&msg.executedwp.order_ref, data + 9, 8);
            std::memcpy(&msg.executedwp.executed_quantity, data + 17, 4);
            std::memcpy(&msg.executedwp.execution_price, data + 21, 4);
        };

        handlers[5] = [](const char *data, ITCHMessage &msg) noexcept
        {
            std::memcpy(&msg.del.message_type, data, 1);
            std::memcpy(&msg.del.timestamp, data + 1, 8);
            std::memcpy(&msg.del.order_ref, data + 9, 8);
        };

        handlers[6] = [](const char *data, ITCHMessage &msg) noexcept
        {
            std::memcpy(&msg.trade.message_type, data, 1);
            std::memcpy(&msg.trade.timestamp, data + 1, 8);
            std::memcpy(&msg.trade.order_ref, data + 9, 8);
            std::memcpy(&msg.trade.stock, data + 17, 8);
            std::memcpy(&msg.trade.quantity, data + 25, 4);
            std::memcpy(&msg.trade.price, data + 29, 4);
            std::memcpy(&msg.trade.match_id, data + 33, 4);
        };

        handlers[7] = [](const char *data, ITCHMessage &msg) noexcept
        {
            std::memcpy(&msg.systemevent.message_type, data, 1);
            std::memcpy(&msg.systemevent.timestamp, data + 1, 8);
            std::memcpy(&msg.systemevent.event_code, data + 9, 1);
        };

        return handlers;
    }

    static inline std::array<MessageHandlerFunc, MAX_MESSAGES> MessageHandlers = makeMessageHandlersLookup();

public:
    void parse(const char *data) noexcept
    {
        MessageHandlers[MessageIndex(*data)](data, ITCHmsg_);
    }
};
