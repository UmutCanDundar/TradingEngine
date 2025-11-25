#pragma once

#include "Parser_FIX.h"
#include "Parser_ITCH_NASDAQ.h"
#include "Parser_ITCH_BIST.h"
#include "Parser_OUCH_BIST.h"
#include "Parser_SBE.h"
#include "Receiver.h"

#include <variant>
#include <boost/lockfree/spsc_queue.hpp>

class Order;

template <typename T>
struct MessageWithVenue
{
    T msg;
    Venue venue;

    MessageWithVenue() noexcept = default;
    MessageWithVenue(const T msg, Venue ven) noexcept : msg(msg), venue(ven) {}
};

inline constexpr size_t DB_QUEUE_CAPACITY = 512;

using MessageTypes_t = std::variant<FIXMessage *, BIST::ITCHMessage, BIST::OUCHMessage, NASDAQ::ITCHMessage, SBEMessage>;
using spscMessageQueue_t = boost::lockfree::spsc_queue<MessageWithVenue<MessageTypes_t>, boost::lockfree::capacity<MESSAGE_QUEUE_CAPACITY>>;
using spscFIXOutSessionQueue_t = boost::lockfree::spsc_queue<FIXSessionMessage*, boost::lockfree::capacity<MESSAGE_QUEUE_CAPACITY>>;
using spscDbQueue_t = boost::lockfree::spsc_queue<std::variant<Order *, MessageWithVenue<FIXMessage *>, MessageWithVenue<BIST::ITCHMessage>, MessageWithVenue<BIST::OUCHMessage>, MessageWithVenue<NASDAQ::ITCHMessage>, MessageWithVenue<SBEMessage>>, boost::lockfree::capacity<DB_QUEUE_CAPACITY>>;
using DbData_t = std::variant<Order *, MessageWithVenue<FIXMessage *>, MessageWithVenue<BIST::ITCHMessage>, MessageWithVenue<BIST::OUCHMessage>, MessageWithVenue<NASDAQ::ITCHMessage>, MessageWithVenue<SBEMessage>>;

class Parser_Dispatch
{
private:
    static constexpr uint16_t DB_QUEUE_THRESHOLD = 256;
    static inline uint16_t PktCount = 1;

    Parser_FIX fixparser_{session_, parser_to_fixbuilder_in_};
    Parser_ITCH_NASDAQ itchparser_nasdaq_;
    Parser_ITCH_BIST itchparser_bist_;
    Parser_OUCH_BIST ouchparser_bist_;
    Parser_SBE sbeparser_;
    
    using ParserFunc = void (Parser_Dispatch::*)(Packet*) noexcept;
    std::array<std::array<ParserFunc, VENUE_COUNT>, PROTOCOL_COUNT> makeParserLookUpTable() noexcept; 
    std::array<std::array<ParserFunc, VENUE_COUNT>, PROTOCOL_COUNT> parser_table_;

    spscPacketQueue_t &receiver_to_parser_;
    spscMessageQueue_t &parser_to_store_;
    spscFIXOutSessionQueue_t &parser_to_fixbuilder_out_;
    spscFIXInSessionQueue_t &parser_to_fixbuilder_in_;
    Session_FIX &session_;
    spscDbQueue_t &db_to_parser_;

public:
    Parser_Dispatch(spscPacketQueue_t &receiver_to_parser, spscMessageQueue_t &parser_to_store, spscFIXOutSessionQueue_t &parser_to_fixbuilder_out, spscFIXInSessionQueue_t &parser_to_fixbuilder_in, Session_FIX &session, spscDbQueue_t &db_to_parser) noexcept;

    void dispatch() noexcept;

private:

    inline void flush_DbQueue() noexcept 
    {
        while (!db_to_parser_.empty())
        {
           DbData_t DbQueueItem; 
           db_to_parser_.pop(DbQueueItem);
           std::visit([&](auto& item)
           {
                using T = std::decay_t<decltype(item)>;
                if constexpr (std::is_same_v<T, MessageWithVenue<FIXMessage*>>)
                {
                    fixparser_.releaseFIX(item.msg);
                }
                else if constexpr (std::is_same_v<T, MessageWithVenue<BIST::ITCHMessage>>)
                {
                    itchparser_bist_.releaseITCH(item.msg);
                }
                else if constexpr (std::is_same_v<T, MessageWithVenue<BIST::OUCHMessage>>)
                {
                    ouchparser_bist_.releaseOUCH(item.msg);
                }
                else if constexpr (std::is_same_v<T, MessageWithVenue<NASDAQ::ITCHMessage>>)
                {
                    itchparser_nasdaq_.releaseITCH(item.msg);
                }
                else if constexpr (std::is_same_v<T, MessageWithVenue<SBEMessage>>)
                {
                    sbeparser_.releaseSBE(item.msg);   
                }
                else if constexpr (std::is_same_v<T, Order*>)
                {
                    return;
                }
            
            }, DbQueueItem);   
        }
    }

    inline void proceedPendingFIX(auto *variant_msg) noexcept
    {
        std::visit([&](auto *msg)
                   {
                       using MsgType = std::remove_pointer_t<decltype(msg)>;

                       if constexpr (std::is_same_v<MsgType, FIXMessage>)
                       {
                           parser_to_store_.push(MessageWithVenue<MessageTypes_t>(msg, Venue::BIST));
                       }
                       else
                       {
                           if (msg->msg_type != static_cast<uint8_t>(FIXTypes::Logon) && !fixparser_.handle_sesMsg(msg))
                               parser_to_fixbuilder_out_.push(msg);
                           else
                               fixparser_.releaseFIX(msg);
                       }

                       fixparser_.pop_pending(msg->seqnum);
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
                    proceedPendingFIX(variant_msg);
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
                    proceedPendingFIX(variant_msg);
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
    }

    inline void parseOUCH_BIST(Packet *pkt) noexcept
    {
        BIST::OUCHMessage ouchMsg{ouchparser_bist_.parse(pkt->data.data())};
        parser_to_store_.push(MessageWithVenue<MessageTypes_t>(ouchMsg, Venue::BIST));
    }

    inline void parseITCH_NASDAQ(Packet *pkt) noexcept
    {
        NASDAQ::ITCHMessage itchMsg{itchparser_nasdaq_.parse(pkt->data.data())};
        parser_to_store_.push(MessageWithVenue<MessageTypes_t>(itchMsg, Venue::NASDAQ));
    }

    inline void parseSBE(Packet *pkt) noexcept
    {
        SBEMessage sbeMsg{sbeparser_.parse(pkt->data.data())};
        parser_to_store_.push(MessageWithVenue<MessageTypes_t>(sbeMsg, pkt->venue));
    }
};
