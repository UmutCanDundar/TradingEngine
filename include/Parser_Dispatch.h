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
using spscFIXSessionQueue_t = boost::lockfree::spsc_queue<FIXSessionMessage*, boost::lockfree::capacity<MESSAGE_QUEUE_CAPACITY>>;

class Parser_Dispatch
{
private:
    Parser_FIX fixparser_{session_};
    Parser_ITCH_NASDAQ itchparser_nasdaq_;
    Parser_ITCH_BIST itchparser_bist_;
    Parser_SBE sbeparser_;
    
    using ParserFunc = void (*)(Parser_Dispatch*, Packet*) noexcept;
    static std::array<std::array<ParserFunc, VENUE_COUNT>, PROTOCOL_COUNT> makeParserLookUpTable() noexcept; 
    static std::array<std::array<ParserFunc, VENUE_COUNT>, PROTOCOL_COUNT> parser_table_;

    spscPacketQueue_t &receiver_to_parser_;
    spscMessageQueue_t &parser_to_store_;
    spscFIXSessionQueue_t &parser_to_fixbuilder_;
    Session_FIX &session_;

public:

    Parser_Dispatch(spscPacketQueue_t &receiver_to_parser, spscMessageQueue_t &parser_to_store, spscFIXSessionQueue_t &parser_to_fixbuilder, Session_FIX& session) noexcept;

    void dispatch() noexcept;

private:

    static inline void proceedPendingFIX(auto& parser_to_store, auto& parser_to_fixbuilder, auto& fixparser, auto* variant_msg) noexcept
    {
        std::visit([&](auto* msg)
                   {
                       using MsgType = std::remove_pointer_t<decltype(msg)>;

                       if constexpr (std::is_same_v<MsgType, FIXMessage>)
                       {
                           parser_to_store.push(MessageWithVenue<MessageTypes_t>(msg, Venue::BIST));
                       }
                       else
                       {
                           if (!fixparser.handle_sesMsg(msg))
                               parser_to_fixbuilder.push(msg);
                           else
                               fixparser.releaseFIX(msg);
                       }
                     
                       fixparser.pop_pending(msg->seqnum);

                   }, *variant_msg);
    }

    static inline void parseFIX(Parser_Dispatch* pd, Packet* pkt) noexcept
    {
        using V_t = std::variant<FIXMessage*, FIXSessionMessage*>;

        auto &fixparser = pd->fixparser_;
        auto &parser_to_store = pd->parser_to_store_;
        auto &parser_to_fixbuilder = pd->parser_to_fixbuilder_;

        FIXMessage* fixMsg{nullptr};
        FIXSessionMessage* fixSesMsg{nullptr};

        const auto type = fixparser.find_type(pkt->data.data());
        
        if(type > 65) 
        {
            fixMsg = fixparser.parse<FIXMessage>(pkt->data.data(), pkt->data.size());
            if(fixparser.is_expected_seqnum(fixMsg->seqnum)) 
            {
                parser_to_store.push(MessageWithVenue<MessageTypes_t>(fixMsg, Venue::BIST));
                fixparser.increase_expected();

                while (V_t* variant_msg = fixparser.find_in_pending(fixparser.get_expected())) 
                {
                    proceedPendingFIX(parser_to_store, parser_to_fixbuilder, fixparser, variant_msg);
                    fixparser.increase_expected();
                }
            }
            else
            {
                fixparser.push_pending(fixMsg);
            }
        }
        else
        {
            fixSesMsg = fixparser.parse<FIXSessionMessage>(pkt->data.data(), pkt->data.size());
            if (fixparser.is_expected_seqnum(fixSesMsg->seqnum))
            {
                if(!fixparser.handle_sesMsg(fixSesMsg)) 
                    parser_to_fixbuilder.push(fixSesMsg);
                else 
                    fixparser.releaseFIX(fixSesMsg);
                
                fixparser.increase_expected();
               
                while (V_t *variant_msg = fixparser.find_in_pending(fixparser.get_expected()))
                {
                    proceedPendingFIX(parser_to_store, parser_to_fixbuilder, fixparser, variant_msg);
                    fixparser.increase_expected();
                }
            }
            else
            {
                fixparser.push_pending(fixSesMsg);
            }
        }
    }

    static inline void parseITCH_BIST(Parser_Dispatch* pd, Packet *pkt) noexcept
    {
        BIST::ITCHMessage itchMsg{pd->itchparser_bist_.parse(pkt->data.data())};
        pd->parser_to_store_.push(MessageWithVenue<MessageTypes_t>(itchMsg, Venue::BIST));
        pd->itchparser_bist_.releaseITCH(itchMsg);
    }

    static inline void parseITCH_NASDAQ(Parser_Dispatch* pd, Packet *pkt) noexcept
    {
        NASDAQ::ITCHMessage itchMsg{pd->itchparser_nasdaq_.parse(pkt->data.data())};
        pd->parser_to_store_.push(MessageWithVenue<MessageTypes_t>(itchMsg, Venue::NASDAQ));
        pd->itchparser_nasdaq_.releaseITCH(itchMsg);
    }

    static inline void parseSBE(Parser_Dispatch* pd, Packet *pkt) noexcept
    {
        SBEMessage sbeMsg{pd->sbeparser_.parse(pkt->data.data())};
        pd->parser_to_store_.push(MessageWithVenue<MessageTypes_t>(sbeMsg, pkt->venue));
        pd->sbeparser_.releaseSBE(sbeMsg);
    }
};
