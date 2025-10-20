#pragma once

#include "Parser_FIX.h"
#include "Parser_ITCH_NASDAQ.h"
#include "Parser_ITCH_BIST.h"
#include "Parser_SBE.h"
#include "Receiver.h"

#include <variant>
#include <boost/lockfree/spsc_queue.hpp>

template <typename T>
struct MessageWithVenue
{
    T msg;
    Venue venue;

    MessageWithVenue() noexcept = default;
    MessageWithVenue(const T msg, Venue ven) noexcept : msg(msg), venue(ven) {}
};

using MessageTypes_t = std::variant<FIXMessage *, BIST::ITCHMessage, NASDAQ::ITCHMessage, SBEMessage>;

inline constexpr size_t MESSAGE_QUEUE_CAPACITY = 1024;
using spscMessageQueue_t = boost::lockfree::spsc_queue<MessageWithVenue<MessageTypes_t>, boost::lockfree::capacity<MESSAGE_QUEUE_CAPACITY>>;

class Parser_Dispatch
{
private:
    Parser_FIX fixparser_;
    Parser_ITCH_NASDAQ itchparser_nasdaq_;
    Parser_ITCH_BIST itchparser_bist_;
    Parser_SBE sbeparser_;
    spscPacketQueue_t &receiver_to_parser_;
    spscMessageQueue_t &parser_to_store_;

    using ParserFunc = void (*)(Parser_Dispatch*, Packet*) noexcept;
    static std::array<std::array<ParserFunc, VENUE_COUNT>,PROTOCOL_COUNT> makeParserLookUpTable() noexcept; 
    static std::array<std::array<ParserFunc, VENUE_COUNT>, PROTOCOL_COUNT> parser_table_;

public:
    Parser_Dispatch(spscPacketQueue_t &receiver_to_parser, spscMessageQueue_t &parser_to_store);

    void dispatch() noexcept;

private:
    static inline void parseFIX(Parser_Dispatch* pd, Packet* pkt) noexcept
    {
        FIXMessage *fixMsg{pd->fixparser_.parse(pkt->data.data(), pkt->data.size())};
        pd->parser_to_store_.push(MessageWithVenue<MessageTypes_t>(fixMsg, pkt->venue));
        pd->fixparser_.releaseFIX(fixMsg);
    }

    static inline void parseITCH_BIST(Parser_Dispatch *pd, Packet *pkt) noexcept
    {
        BIST::ITCHMessage itchMsg{pd->itchparser_bist_.parse(pkt->data.data())};
        pd->parser_to_store_.push(MessageWithVenue<MessageTypes_t>(itchMsg, pkt->venue));
        pd->itchparser_bist_.releaseITCH(itchMsg);
    }

    static inline void parseITCH_NASDAQ(Parser_Dispatch *pd, Packet *pkt) noexcept
    {
        NASDAQ::ITCHMessage itchMsg{pd->itchparser_nasdaq_.parse(pkt->data.data())};
        pd->parser_to_store_.push(MessageWithVenue<MessageTypes_t>(itchMsg, pkt->venue));
        pd->itchparser_nasdaq_.releaseITCH(itchMsg);
    }

    static inline void parseSBE(Parser_Dispatch *pd, Packet *pkt) noexcept
    {
        SBEMessage sbeMsg{pd->sbeparser_.parse(pkt->data.data())};
        pd->parser_to_store_.push(MessageWithVenue<MessageTypes_t>(sbeMsg, pkt->venue));
        pd->sbeparser_.releaseSBE(sbeMsg);
    }
};
