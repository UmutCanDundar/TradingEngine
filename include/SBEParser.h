#pragma once

#include "BaseParser.h"
#include "GeneratedSBEMessages.h"
#include <cstdint>
#include <cstring>
#include <array>
#include <variant>

using SBEMessage = std::variant<
    SBEAddOrderMessage,
    SBEDeleteOrderMessage,
    SBETradeMessage,
    SBEModifyOrderMessage,
    SBEHeartbeatMessage,
    SBEMarketStatusMessage,
    SBEInstrumentDefinitionMessage,
    SBESequenceResetMessage>;

class SBEParser
{
private:
    static constexpr size_t MAX_MESSAGES = 8;

    using MessageHandlerFunc = void (*)(const char *, SBEMessage &) noexcept;

    static inline std::array<MessageHandlerFunc, MAX_MESSAGES> makeMessageHandlersLookup() noexcept
    {
        std::array<MessageHandlerFunc, MAX_MESSAGES> handlers{};

        handlers[0] = [](const char *data, SBEMessage &msg) noexcept
        {
            SBEAddOrderMessage m{};
            std::memcpy(&m.header, data, sizeof(SBEHeader));
            std::memcpy(&m.orderId, data + 8, 8);
            std::memcpy(&m.price, data + 16, 4);
            std::memcpy(&m.quantity, data + 20, 4);
            std::memcpy(&m.side, data + 24, 1);
            msg = m;
        };

        handlers[1] = [](const char *data, SBEMessage &msg) noexcept
        {
            SBEDeleteOrderMessage m{};
            std::memcpy(&m.header, data, sizeof(SBEHeader));
            std::memcpy(&m.orderId, data + 8, 8);
            msg = m;
        };

        handlers[2] = [](const char *data, SBEMessage &msg) noexcept
        {
            SBETradeMessage m{};
            std::memcpy(&m.header, data, sizeof(SBEHeader));
            std::memcpy(&m.tradeId, data + 8, 8);
            std::memcpy(&m.orderId, data + 16, 8);
            std::memcpy(&m.price, data + 24, 4);
            std::memcpy(&m.quantity, data + 28, 4);
            std::memcpy(&m.aggressorSide, data + 32, 1);
            msg = m;
        };

        handlers[3] = [](const char *data, SBEMessage &msg) noexcept
        {
            SBEModifyOrderMessage m{};
            std::memcpy(&m.header, data, sizeof(SBEHeader));
            std::memcpy(&m.orderId, data + 8, 8);
            std::memcpy(&m.newQuantity, data + 16, 4);
            msg = m;
        };

        handlers[4] = [](const char *data, SBEMessage &msg) noexcept
        {
            SBEHeartbeatMessage m{};
            std::memcpy(&m.header, data, sizeof(SBEHeader));
            std::memcpy(&m.timestamp, data + 8, 8);
            msg = m;
        };

        handlers[5] = [](const char *data, SBEMessage &msg) noexcept
        {
            SBEMarketStatusMessage m{};
            std::memcpy(&m.header, data, sizeof(SBEHeader));
            std::memcpy(&m.instrumentId, data + 8, 8);
            std::memcpy(&m.marketState, data + 16, 1);
            msg = m;
        };

        handlers[6] = [](const char *data, SBEMessage &msg) noexcept
        {
            SBEInstrumentDefinitionMessage m{};
            std::memcpy(&m.header, data, sizeof(SBEHeader));
            std::memcpy(&m.instrumentId, data + 8, 8);
            std::memcpy(&m.lotSize, data + 16, 4);
            std::memcpy(&m.currencyCode, data + 20, 3);
            msg = m;
        };

        handlers[7] = [](const char *data, SBEMessage &msg) noexcept
        {
            SBESequenceResetMessage m{};
            std::memcpy(&m.header, data, sizeof(SBEHeader));
            std::memcpy(&m.newSeqNo, data + 8, 8);
            msg = m;
        };

        return handlers;
    }

    static inline std::array<MessageHandlerFunc, MAX_MESSAGES> MessageHandlers = makeMessageHandlersLookup();

public:
    SBEMessage parse(const char *data) noexcept
    {
        SBEMessage SBEmsg_;
        uint16_t templateId = static_cast<uint8_t>(data[2]) | (static_cast<uint8_t>(data[3]) << 8);
        size_t index = MessageIndex(templateId);
        if (index != 99)
            MessageHandlers[index](data, SBEmsg_);
        return SBEmsg_;
    }
};
