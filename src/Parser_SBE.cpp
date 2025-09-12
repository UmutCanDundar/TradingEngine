#include "Parser_SBE.h"

MessagePools<SBEMessageTypes> Parser_SBE::sbe_pools_;

Parser_SBE::Parser_SBE() noexcept
{
    sbe_pools_.initialize_all();
}

std::array<Parser_SBE::MessageHandlerFunc, Parser_SBE::MAX_MESSAGES> Parser_SBE::makeMessageHandlersLookup() noexcept
{
    std::array<MessageHandlerFunc, MAX_MESSAGES> handlers{};

    handlers[0] = [](const char *data, SBEMessage &msg) noexcept
    {
        SBEAddOrderMessage *m = nullptr;
        sbe_pools_.get_pool<SBEAddOrderMessage>().acquire(m);

        std::memcpy(&m->header, data, sizeof(SBEHeader));
        std::memcpy(&m->orderId, data + 8, 8);
        std::memcpy(&m->price, data + 16, 4);
        std::memcpy(&m->quantity, data + 20, 4);
        std::memcpy(&m->side, data + 24, 1);
        msg = m;
    };

    handlers[1] = [](const char *data, SBEMessage &msg) noexcept
    {
        SBEDeleteOrderMessage *m = nullptr;
        sbe_pools_.get_pool<SBEDeleteOrderMessage>().acquire(m);

        std::memcpy(&m->header, data, sizeof(SBEHeader));
        std::memcpy(&m->orderId, data + 8, 8);
        msg = m;
    };

    handlers[2] = [](const char *data, SBEMessage &msg) noexcept
    {
        SBETradeMessage *m = nullptr;
        sbe_pools_.get_pool<SBETradeMessage>().acquire(m);

        std::memcpy(&m->header, data, sizeof(SBEHeader));
        std::memcpy(&m->tradeId, data + 8, 8);
        std::memcpy(&m->orderId, data + 16, 8);
        std::memcpy(&m->price, data + 24, 4);
        std::memcpy(&m->quantity, data + 28, 4);
        std::memcpy(&m->aggressorSide, data + 32, 1);
        msg = m;
    };

    handlers[3] = [](const char *data, SBEMessage &msg) noexcept
    {
        SBEModifyOrderMessage *m = nullptr;
        sbe_pools_.get_pool<SBEModifyOrderMessage>().acquire(m);

        std::memcpy(&m->header, data, sizeof(SBEHeader));
        std::memcpy(&m->orderId, data + 8, 8);
        std::memcpy(&m->newQuantity, data + 16, 4);
        msg = m;
    };

    handlers[4] = [](const char *data, SBEMessage &msg) noexcept
    {
        SBEHeartbeatMessage *m = nullptr;
        sbe_pools_.get_pool<SBEHeartbeatMessage>().acquire(m);

        std::memcpy(&m->header, data, sizeof(SBEHeader));
        std::memcpy(&m->timestamp, data + 8, 8);
        msg = m;
    };

    handlers[5] = [](const char *data, SBEMessage &msg) noexcept
    {
        SBEMarketStatusMessage *m = nullptr;
        sbe_pools_.get_pool<SBEMarketStatusMessage>().acquire(m);

        std::memcpy(&m->header, data, sizeof(SBEHeader));
        std::memcpy(&m->instrumentId, data + 8, 8);
        std::memcpy(&m->marketState, data + 16, 1);
        msg = m;
    };

    handlers[6] = [](const char *data, SBEMessage &msg) noexcept
    {
        SBEInstrumentDefinitionMessage *m = nullptr;
        sbe_pools_.get_pool<SBEInstrumentDefinitionMessage>().acquire(m);

        std::memcpy(&m->header, data, sizeof(SBEHeader));
        std::memcpy(&m->instrumentId, data + 8, 8);
        std::memcpy(&m->lotSize, data + 16, 4);
        std::memcpy(&m->currencyCode, data + 20, 3);
        msg = m;
    };

    handlers[7] = [](const char *data, SBEMessage &msg) noexcept
    {
        SBESequenceResetMessage *m = nullptr;
        sbe_pools_.get_pool<SBESequenceResetMessage>().acquire(m);

        std::memcpy(&m->header, data, sizeof(SBEHeader));
        std::memcpy(&m->newSeqNo, data + 8, 8);
        msg = m;
    };

    return handlers;
}

std::array<Parser_SBE::MessageHandlerFunc, Parser_SBE::MAX_MESSAGES> Parser_SBE::MessageHandlers = makeMessageHandlersLookup();