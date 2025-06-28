#pragma once

#include "BaseParser.h"
#include "GeneratedSBEMessages.h"
#include <cstdint>
#include <cstring>
#include <array>

union alignas(64) SBEMessage
{
    SBEAddOrderMessage addorder;
    SBEDeleteOrderMessage deleteorder;
    SBETradeMessage trade;
    SBEModifyOrderMessage modifyorder;
    SBEHeartbeatMessage heartbeat;
    SBEMarketStatusMessage marketstatus;
    SBEInstrumentDefinitionMessage instrumentdef;
    SBESequenceResetMessage seqreset;
};

class SBEParser
{
private:
    static constexpr size_t MAX_MESSAGES = 8;
    SBEMessage SBEmsg_{};

    using MessageHandlerFunc = void (*)(const char *, SBEMessage &) noexcept;

    static inline std::array<MessageHandlerFunc, MAX_MESSAGES> makeMessageHandlersLookup() noexcept
    {
        alignas(64) std::array<MessageHandlerFunc, MAX_MESSAGES> handlers{};

        handlers[0] = [](const char *data, SBEMessage &msg) noexcept
        {
            std::memcpy(&msg.addorder.header, data, sizeof(SBEHeader));
            std::memcpy(&msg.addorder.orderId, data + 8, 8);
            std::memcpy(&msg.addorder.price, data + 16, 4);
            std::memcpy(&msg.addorder.quantity, data + 20, 4);
            std::memcpy(&msg.addorder.side, data + 24, 1);
        };

        handlers[1] = [](const char *data, SBEMessage &msg) noexcept
        {
            std::memcpy(&msg.deleteorder.header, data, sizeof(SBEHeader));
            std::memcpy(&msg.deleteorder.orderId, data + 8, 8);
        };

        handlers[2] = [](const char *data, SBEMessage &msg) noexcept
        {
            std::memcpy(&msg.trade.header, data, sizeof(SBEHeader));
            std::memcpy(&msg.trade.tradeId, data + 8, 8);
            std::memcpy(&msg.trade.orderId, data + 16, 8);
            std::memcpy(&msg.trade.price, data + 24, 4);
            std::memcpy(&msg.trade.quantity, data + 28, 4);
            std::memcpy(&msg.trade.aggressorSide, data + 32, 1);
        };

        handlers[3] = [](const char *data, SBEMessage &msg) noexcept
        {
            std::memcpy(&msg.modifyorder.header, data, sizeof(SBEHeader));
            std::memcpy(&msg.modifyorder.orderId, data + 8, 8);
            std::memcpy(&msg.modifyorder.newQuantity, data + 16, 4);
        };

        handlers[4] = [](const char *data, SBEMessage &msg) noexcept
        {
            std::memcpy(&msg.heartbeat.header, data, sizeof(SBEHeader));
            std::memcpy(&msg.heartbeat.timestamp, data + 8, 8);
        };

        handlers[5] = [](const char *data, SBEMessage &msg) noexcept
        {
            std::memcpy(&msg.marketstatus.header, data, sizeof(SBEHeader));
            std::memcpy(&msg.marketstatus.instrumentId, data + 8, 8);
            std::memcpy(&msg.marketstatus.marketState, data + 16, 1);
        };

        handlers[6] = [](const char *data, SBEMessage &msg) noexcept
        {
            std::memcpy(&msg.instrumentdef.header, data, sizeof(SBEHeader));
            std::memcpy(&msg.instrumentdef.instrumentId, data + 8, 8);
            std::memcpy(&msg.instrumentdef.lotSize, data + 16, 4);
            std::memcpy(&msg.instrumentdef.currencyCode, data + 20, 3);
        };

        handlers[7] = [](const char *data, SBEMessage &msg) noexcept
        {
            std::memcpy(&msg.seqreset.header, data, sizeof(SBEHeader));
            std::memcpy(&msg.seqreset.newSeqNo, data + 8, 8);
        };

        return handlers;
    }

    static inline std::array<MessageHandlerFunc, MAX_MESSAGES> MessageHandlers = makeMessageHandlersLookup();

public:
    void parse(const char *data) noexcept
    {
        uint16_t templateId = data[2] | (data[3] << 8);
        MessageHandlers[MessageIndex(templateId)](data, SBEmsg_);
    }
};
