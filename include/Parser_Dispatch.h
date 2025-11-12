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
using spscMessageQueue_t = boost::lockfree::spsc_queue<MessageWithVenue<MessageTypes_t>, boost::lockfree::capacity<MESSAGE_QUEUE_CAPACITY>>;
using spscFIXOutSessionQueue_t = boost::lockfree::spsc_queue<FIXSessionMessage*, boost::lockfree::capacity<MESSAGE_QUEUE_CAPACITY>>;

class Parser_Dispatch
{
private:
    Parser_FIX fixparser_{session_, parser_to_fixbuilder_in_};
    Parser_ITCH_NASDAQ itchparser_nasdaq_;
    Parser_ITCH_BIST itchparser_bist_;
    Parser_SBE sbeparser_;
    
    using ParserFunc = void (Parser_Dispatch::*)(Packet*) noexcept;
    std::array<std::array<ParserFunc, VENUE_COUNT>, PROTOCOL_COUNT> makeParserLookUpTable() noexcept; 
    std::array<std::array<ParserFunc, VENUE_COUNT>, PROTOCOL_COUNT> parser_table_;

    spscPacketQueue_t &receiver_to_parser_;
    spscMessageQueue_t &parser_to_store_;
    spscFIXOutSessionQueue_t &parser_to_fixbuilder_out_;
    spscFIXInSessionQueue_t &parser_to_fixbuilder_in_;
    Session_FIX &session_;

public:
    Parser_Dispatch(spscPacketQueue_t &receiver_to_parser, spscMessageQueue_t &parser_to_store, spscFIXOutSessionQueue_t &parser_to_fixbuilder_out, spscFIXInSessionQueue_t &parser_to_fixbuilder_in, Session_FIX &session) noexcept;

    void dispatch() noexcept;

private:

    inline void proceedPendingFIX(auto& parser_to_store, auto& parser_to_fixbuilder_out, auto& fixparser, auto *variant_msg) noexcept
    {
        std::visit([&](auto *msg)
                   {
                       using MsgType = std::remove_pointer_t<decltype(msg)>;

                       if constexpr (std::is_same_v<MsgType, FIXMessage>)
                       {
                           parser_to_store.push(MessageWithVenue<MessageTypes_t>(msg, Venue::BIST));
                       }
                       else
                       {
                           if (msg->msg_type != static_cast<uint8_t>(FIXTypes::Logon) && !fixparser.handle_sesMsg(msg))
                               parser_to_fixbuilder_out.push(msg);
                           else
                               fixparser.releaseFIX(msg);
                       }

                       fixparser.pop_pending(msg->seqnum);
                   },
                   *variant_msg);
    }

    inline void parseFIX(Packet* pkt) noexcept
    {
        using V_t = std::variant<FIXMessage*, FIXSessionMessage*>;

        auto expected = session_.get_expected();

        FIXMessage* fixMsg{nullptr};
        FIXSessionMessage* fixSesMsg{nullptr};

        const auto type = fixparser_.find_type(pkt->data.data());
        
        if(type > 65) 
        {
            fixMsg = fixparser_.parse<FIXMessage>(pkt->data.data(), pkt->data.size());
            if(expected == fixMsg->seqnum) 
            {
                parser_to_store_.push(MessageWithVenue<MessageTypes_t>(fixMsg, Venue::BIST));
                session_.increase_expected_seq();

                while (V_t *variant_msg = fixparser_.find_in_pending(expected))
                {
                    proceedPendingFIX(parser_to_store_, parser_to_fixbuilder_out_, fixparser_, variant_msg);
                    session_.increase_expected_seq();
                }
            }
            else
            {
                fixparser_.resend_logic(fixMsg->seqnum);
                fixparser_.push_pending(fixMsg);
            }
        }
        else
        {
            fixSesMsg = fixparser_.parse<FIXSessionMessage>(pkt->data.data(), pkt->data.size());
            if (expected == fixMsg->seqnum)
            {
                if (!fixparser_.handle_sesMsg(fixSesMsg))
                    parser_to_fixbuilder_out_.push(fixSesMsg);
                else
                    fixparser_.releaseFIX(fixSesMsg);

                session_.increase_expected_seq();

                while (V_t *variant_msg = fixparser_.find_in_pending(expected))
                {
                    proceedPendingFIX(parser_to_store_, parser_to_fixbuilder_out_, fixparser_, variant_msg);
                    session_.increase_expected_seq();
                }
            }
            else
            {
                if (LIKELY(fixSesMsg->msg_type != static_cast<uint8_t>(FIXTypes::Logon))) 
                {
                    fixparser_.resend_logic(fixSesMsg->seqnum);
                    fixparser_.push_pending(fixSesMsg);
                }
                else
                {
                    fixparser_.resend_logic_logon(fixSesMsg);
                }
            }
        }
    }

    inline void parseITCH_BIST(Packet *pkt) noexcept
    {
        BIST::ITCHMessage itchMsg{itchparser_bist_.parse(pkt->data.data())};
        parser_to_store_.push(MessageWithVenue<MessageTypes_t>(itchMsg, Venue::BIST));
        itchparser_bist_.releaseITCH(itchMsg);
    }

    inline void parseITCH_NASDAQ(Packet *pkt) noexcept
    {
        NASDAQ::ITCHMessage itchMsg{itchparser_nasdaq_.parse(pkt->data.data())};
        parser_to_store_.push(MessageWithVenue<MessageTypes_t>(itchMsg, Venue::NASDAQ));
        itchparser_nasdaq_.releaseITCH(itchMsg);
    }

    inline void parseSBE(Packet *pkt) noexcept
    {
        SBEMessage sbeMsg{sbeparser_.parse(pkt->data.data())};
        parser_to_store_.push(MessageWithVenue<MessageTypes_t>(sbeMsg, pkt->venue));
        sbeparser_.releaseSBE(sbeMsg);
    }
};
