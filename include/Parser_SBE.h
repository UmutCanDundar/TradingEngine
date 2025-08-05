#pragma once

#include "BaseParser.h"
#include "GeneratedSBEMessages.h"
#include "MessagePool.h"

#include <cstdint>
#include <cstring>
#include <array>
#include <variant>

using SBEMessageTypes = std::tuple<
    SBEAddOrderMessage,
    SBEDeleteOrderMessage,
    SBETradeMessage,
    SBEModifyOrderMessage,
    SBEHeartbeatMessage,
    SBEMarketStatusMessage,
    SBEInstrumentDefinitionMessage,
    SBESequenceResetMessage>;

using SBEMessage = std::variant<
    SBEAddOrderMessage *,
    SBEDeleteOrderMessage *,
    SBETradeMessage *,
    SBEModifyOrderMessage *,
    SBEHeartbeatMessage *,
    SBEMarketStatusMessage *,
    SBEInstrumentDefinitionMessage *,
    SBESequenceResetMessage *>;

class Parser_SBE
{
private:
    static constexpr size_t MAX_MESSAGES = 8;

    static MessagePools<SBEMessageTypes> sbe_pools_;

    using MessageHandlerFunc = void (*)(const char *, SBEMessage &) noexcept;
    static std::array<MessageHandlerFunc, MAX_MESSAGES> makeMessageHandlersLookup() noexcept;
    static std::array<MessageHandlerFunc, MAX_MESSAGES> MessageHandlers;

public:
    Parser_SBE() noexcept;

    template <typename T>
    inline void release(T *sbeMsg)
    {
        sbe_pools_.get_pool<T>().release(sbeMsg);
    }

    inline void releaseSBE(SBEMessage &sbeMsg)
    {
        std::visit([this](auto *ptr)
                   {
                       using T = std::remove_pointer_t<decltype(ptr)>;
                       release<T>(ptr); },
                   sbeMsg);
    }

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
