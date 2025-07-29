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

class Parser_SBE
{
private:
    static constexpr size_t MAX_MESSAGES = 8;

    using MessageHandlerFunc = void (*)(const char *, SBEMessage &) noexcept;

    static std::array<MessageHandlerFunc, MAX_MESSAGES> makeMessageHandlersLookup() noexcept;

    static std::array<MessageHandlerFunc, MAX_MESSAGES> MessageHandlers;

public:
    inline SBEMessage parse(const char *data) noexcept
    {
        SBEMessage SBEmsg_;
        uint16_t templateId = static_cast<uint8_t>(data[2]) | (static_cast<uint8_t>(data[3]) << 8);
        size_t index = MessageIndex(templateId);
        if (index != 99)
            MessageHandlers[index](data, SBEmsg_);
        return SBEmsg_;
    }
};
