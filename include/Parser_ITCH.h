#pragma once

#include "GeneratedITCHMessages.h"
#include "Logger.h"
#include "MessagePool.h"

#include <cstdint>
#include <cstring>
#include <array>
#include <string_view>
#include <variant>

using ITCHMessageTypes = std::tuple<
    ITCHAddOrderMessage,
    ITCHAddOrderMPIDMessage,
    ITCHCancelMessage,
    ITCHExecutedMessage,
    ITCHExecutedWithPriceMessage,
    ITCHDeleteMessage,
    ITCHReplaceMessage,
    ITCHTradeMessage,
    ITCHSystemEventMessage,
    ITCHStockDirectoryMessage,
    ITCHTradingStateMessage>;

using ITCHMessage = std::variant<
    ITCHAddOrderMessage *,
    ITCHAddOrderMPIDMessage *,
    ITCHCancelMessage *,
    ITCHExecutedMessage *,
    ITCHExecutedWithPriceMessage *,
    ITCHDeleteMessage *,
    ITCHReplaceMessage *,
    ITCHTradeMessage *,
    ITCHSystemEventMessage *,
    ITCHStockDirectoryMessage *,
    ITCHTradingStateMessage *>;

class Parser_ITCH
{
private:
    static constexpr size_t MAX_MESSAGES = 8;

    static MessagePools<ITCHMessageTypes> itch_pools_;

    using MessageHandlerFunc = void (*)(const char *, ITCHMessage &) noexcept;
    static std::array<MessageHandlerFunc, MAX_MESSAGES> makeMessageHandlersLookup() noexcept;
    static std::array<MessageHandlerFunc, MAX_MESSAGES> MessageHandlers;

public:
    Parser_ITCH() noexcept;

    template <typename T>
    inline void release(T *itchMsg)
    {
        itch_pools_.get_pool<T>().release(itchMsg);
    }

    inline void releaseITCH(ITCHMessage &itchMsg)
    {
        std::visit([this](auto *ptr)
                   {
                       using T = std::remove_pointer_t<decltype(ptr)>;
                       release<T>(ptr); },
                   itchMsg);
    }

    inline ITCHMessage parse(const char *data) noexcept
    {
        ITCHMessage ITCHmsg;
        size_t index = MessageIndex(*data);
        if (index != 99)
            MessageHandlers[index](data, ITCHmsg);
        return ITCHmsg;
    }
};
