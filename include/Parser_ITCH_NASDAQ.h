#pragma once

#include "GeneratedITCHMessages_NASDAQ.h"
#include "Logger.h"
#include "MessagePool.h"

#include <cstdint>
#include <cstring>
#include <array>
#include <string_view>
#include <variant>

namespace NASDAQ
{
    using ITCHMessageTypes = std::tuple<
        NASDAQ::ITCHAddOrderMessage,
        NASDAQ::ITCHAddOrderMPIDMessage,
        NASDAQ::ITCHCancelMessage,
        NASDAQ::ITCHExecutedMessage,
        NASDAQ::ITCHExecutedWithPriceMessage,
        NASDAQ::ITCHDeleteMessage,
        NASDAQ::ITCHReplaceMessage,
        NASDAQ::ITCHTradeMessage,
        NASDAQ::ITCHSystemEventMessage,
        NASDAQ::ITCHStockDirectoryMessage,
        NASDAQ::ITCHTradingStateMessage>;

    using ITCHMessage = std::variant<
        NASDAQ::ITCHAddOrderMessage *,
        NASDAQ::ITCHAddOrderMPIDMessage *,
        NASDAQ::ITCHCancelMessage *,
        NASDAQ::ITCHExecutedMessage *,
        NASDAQ::ITCHExecutedWithPriceMessage *,
        NASDAQ::ITCHDeleteMessage *,
        NASDAQ::ITCHReplaceMessage *,
        NASDAQ::ITCHTradeMessage *,
        NASDAQ::ITCHSystemEventMessage *,
        NASDAQ::ITCHStockDirectoryMessage *,
        NASDAQ::ITCHTradingStateMessage *>;
}

    class Parser_ITCH_NASDAQ
    {
    private:
        static constexpr size_t MAX_MESSAGES = 11;

        static MessagePools<NASDAQ::ITCHMessageTypes> itch_pools_;

        using MessageHandlerFunc = void (*)(const char *, NASDAQ::ITCHMessage &) noexcept;
        static std::array<MessageHandlerFunc, MAX_MESSAGES> makeMessageHandlersLookup() noexcept;
        static std::array<MessageHandlerFunc, MAX_MESSAGES> MessageHandlers;

    public:
        Parser_ITCH_NASDAQ() noexcept;

        template <typename T>
        inline void release(T *itchMsg)
        {
            itch_pools_.get_pool<T>().release(itchMsg);
        }

        inline void releaseITCH(NASDAQ::ITCHMessage itchMsg)
        {
            std::visit([this](auto *ptr)
                       {
                       using T = std::remove_pointer_t<decltype(ptr)>;
                       release<T>(ptr); },
                       itchMsg);
        }

        inline NASDAQ::ITCHMessage parse(const char *data) noexcept
        {
            NASDAQ::ITCHMessage ITCHmsg;
            size_t index = static_cast<size_t>(NASDAQ::itchMessageIndex(*data));
            if (index != 99)
                MessageHandlers[index](data, ITCHmsg);
            return ITCHmsg;
        }
    };
