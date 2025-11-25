#pragma once

#include "GeneratedOUCHMessages_BIST.h"
#include "Logger.h"
#include "MessagePool.h"

#include <cstdint>
#include <cstring>
#include <array>
#include <string_view>
#include <variant>

namespace BIST {

    using OUCHMessageTypes = std::tuple<
        BIST::OUT::OUCHOrderAcceptedMessage,
        BIST::OUT::OUCHOrderRejectedMessage,
        BIST::OUT::OUCHOrderReplacedMessage,
        BIST::OUT::OUCHOrderCancelledMessage,
        BIST::OUT::OUCHOrderExecutedMessage>;

    using OUCHMessage = std::variant<
        BIST::OUT::OUCHOrderAcceptedMessage *,
        BIST::OUT::OUCHOrderRejectedMessage *,
        BIST::OUT::OUCHOrderReplacedMessage *,
        BIST::OUT::OUCHOrderCancelledMessage *,
        BIST::OUT::OUCHOrderExecutedMessage *>;
}

class Parser_OUCH_BIST
{
private:
    static constexpr size_t MAX_MESSAGES = 14;

    static MessagePools<BIST::OUCHMessageTypes> ouch_pools_;

    using MessageHandlerFunc = void (*)(const char *, BIST::OUCHMessage &) noexcept;
    static std::array<MessageHandlerFunc, MAX_MESSAGES> makeMessageHandlersLookup() noexcept;
    static std::array<MessageHandlerFunc, MAX_MESSAGES> MessageHandlers;

public:
    Parser_OUCH_BIST() noexcept;

    template <typename T>
    inline void release(T *ouchMsg)
    {
        ouch_pools_.get_pool<T>().release(ouchMsg);
    }

    inline void releaseOUCH(BIST::OUCHMessage &ouchMsg)
    {
        std::visit([this](auto *ptr)
                   {
                       using T = std::remove_pointer_t<decltype(ptr)>;
                       release<T>(ptr); },
                   ouchMsg);
    }

    inline BIST::OUCHMessage parse(const char *data) noexcept
    {
        BIST::OUCHMessage OUCHmsg;
        size_t index = static_cast<size_t>(BIST::ouchMessageIndex(*data));
        if (index != 99)
            MessageHandlers[index](data, OUCHmsg);
        return OUCHmsg;
    }
};
