#pragma once

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

class Parser_ITCH
{
private:
    static constexpr size_t MAX_MESSAGES = 8;

    using MessageHandlerFunc = void (*)(const char *, ITCHMessage &) noexcept;

    static std::array<MessageHandlerFunc, MAX_MESSAGES> makeMessageHandlersLookup() noexcept;

    static std::array<MessageHandlerFunc, MAX_MESSAGES> MessageHandlers;

public:
    inline ITCHMessage parse(const char *data) noexcept
    {
        ITCHMessage ITCHmsg_;
        size_t index = MessageIndex(*data);
        if (index != 99)
            MessageHandlers[index](data, ITCHmsg_);
        return ITCHmsg_;
    }
};
