#pragma once

#include "GeneratedOUCHMessages_NASDAQ.h"
#include "Logger.h"
#include "MessagePool.h"

#include <cstdint>
#include <cstring>
#include <array>
#include <string_view>
#include <variant>

namespace NASDAQ {

    using OUCHMessageTypes = std::tuple<
        NASDAQ::OUT::OUCHSystemEventMessage,
        NASDAQ::OUT::OUCHOrderAcceptedMessage,
        NASDAQ::OUT::OUCHOrderReplacedMessage,
        NASDAQ::OUT::OUCHOrderCancelledMessage,
        NASDAQ::OUT::OUCHOrderExecutedMessage,
        NASDAQ::OUT::OUCHOrderRejectedMessage,
        NASDAQ::OUT::OUCHBrokenTradeMessage,
        NASDAQ::OUT::OUCHOrderModifiedMessage,
        NASDAQ::OUT::OUCHCancelPendingMessage,
        NASDAQ::OUT::OUCHCancelRejectMessage,
        NASDAQ::OUT::OUCHAccountQueryResponse>;

    using OUCHMessage = std::variant<
        NASDAQ::OUT::OUCHSystemEventMessage *,
        NASDAQ::OUT::OUCHOrderAcceptedMessage *,
        NASDAQ::OUT::OUCHOrderReplacedMessage *,
        NASDAQ::OUT::OUCHOrderCancelledMessage *,
        NASDAQ::OUT::OUCHOrderExecutedMessage *,
        NASDAQ::OUT::OUCHOrderRejectedMessage *,
        NASDAQ::OUT::OUCHBrokenTradeMessage *,
        NASDAQ::OUT::OUCHOrderModifiedMessage *,
        NASDAQ::OUT::OUCHCancelPendingMessage *,
        NASDAQ::OUT::OUCHCancelRejectMessage *,
        NASDAQ::OUT::OUCHAccountQueryResponse *>;
}

class Parser_OUCH_NASDAQ
{
private:
    static constexpr size_t MAX_MESSAGES = 14;

    static MessagePools<NASDAQ::OUCHMessageTypes> ouch_pools_;

    using MessageHandlerFunc = void (*)(const char *, NASDAQ::OUCHMessage &) noexcept;
    static const std::array<MessageHandlerFunc, MAX_MESSAGES>& makeMessageHandlersLookup() noexcept;
    static const std::array<MessageHandlerFunc, MAX_MESSAGES>& MessageHandlers;

public:
    Parser_OUCH_NASDAQ() noexcept;

    template <typename T>
    inline void release(T *ouchMsg)
    {
        ouch_pools_.get_pool<T>().release(ouchMsg);
    }

    inline void releaseOUCH(NASDAQ::OUCHMessage &ouchMsg)
    {
        std::visit([this](auto *ptr)
                   {
                       using T = std::remove_pointer_t<decltype(ptr)>;
                       release<T>(ptr); },
                   ouchMsg);
    }

    inline NASDAQ::OUCHMessage parse(const char *data) noexcept
    {
        NASDAQ::OUCHMessage OUCHmsg;
        size_t index = static_cast<size_t>(NASDAQ::ouchMessageIndex(*data));
        if (index != 99)
            MessageHandlers[index](data, OUCHmsg);
        return OUCHmsg;
    }
};
