#pragma once

#include "GeneratedITCHMessages_BIST.h"
#include "Logger.h"
#include "MessagePool.h"

#include <cstdint>
#include <cstring>
#include <array>
#include <string_view>
#include <variant>

namespace BIST {

    using ITCHMessageTypes = std::tuple<
        BIST::ITCHSecondsMessage,
        BIST::ITCHOrderBookDirectoryMessage,
        BIST::ITCHCombinationOrderBookLegMessage,
        BIST::ITCHTickSizeTableEntryMessage,
        BIST::ITCHShortSellStatusMessage,
        BIST::ITCHSystemEventMessage,
        BIST::ITCHOrderBookStateMessage,
        BIST::ITCHAddOrderMessage,
        BIST::ITCHOrderExecutedMessage,
        BIST::ITCHOrderExecutedWithPriceMessage,
        BIST::ITCHOrderDeleteMessage,
        BIST::ITCHOrderBookFlushMessage,
        BIST::ITCHTradeMessage,
        BIST::ITCHEquilibriumPriceUpdateMessage>;

    using ITCHMessage = std::variant<
        BIST::ITCHSecondsMessage *,
        BIST::ITCHOrderBookDirectoryMessage *,
        BIST::ITCHCombinationOrderBookLegMessage *,
        BIST::ITCHTickSizeTableEntryMessage *,
        BIST::ITCHShortSellStatusMessage *,
        BIST::ITCHSystemEventMessage *,
        BIST::ITCHOrderBookStateMessage *,
        BIST::ITCHAddOrderMessage *,
        BIST::ITCHOrderExecutedMessage *,
        BIST::ITCHOrderExecutedWithPriceMessage *,
        BIST::ITCHOrderDeleteMessage *,
        BIST::ITCHOrderBookFlushMessage *,
        BIST::ITCHTradeMessage *,
        BIST::ITCHEquilibriumPriceUpdateMessage *>;                                                                     
}

class Parser_ITCH_BIST
{
private:
    static constexpr size_t MAX_MESSAGES = 14;

    static MessagePools<BIST::ITCHMessageTypes> itch_pools_;

    using MessageHandlerFunc = void (*)(const char *, BIST::ITCHMessage &) noexcept;
    static std::array<MessageHandlerFunc, MAX_MESSAGES> makeMessageHandlersLookup() noexcept;
    static std::array<MessageHandlerFunc, MAX_MESSAGES> MessageHandlers;

public:
    Parser_ITCH_BIST() noexcept;

    template <typename T>
    inline void release(T *itchMsg)
    {
        itch_pools_.get_pool<T>().release(itchMsg);
    }

    inline void releaseITCH(BIST::ITCHMessage itchMsg)
    {
        std::visit([this](auto *ptr)
                   {
                       using T = std::remove_pointer_t<decltype(ptr)>;
                       release<T>(ptr); },
                   itchMsg);
    }

    inline BIST::ITCHMessage parse(const char *data) noexcept
    {
        BIST::ITCHMessage ITCHmsg;
        size_t index = static_cast<size_t>(BIST::itchMessageIndex(*data));
        if (index != 99)
            MessageHandlers[index](data, ITCHmsg);
        return ITCHmsg;
    }
};
