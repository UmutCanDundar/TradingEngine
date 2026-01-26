#pragma once

#include "Parser_FIX.h"
#include "Parser_ITCH_NASDAQ.h"
#include "Parser_ITCH_BIST.h"
#include "Parser_OUCH_BIST.h"
#include "Parser_OUCH_NASDAQ.h"
#include "NetworkIO.h"

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

using MessageTypes_t = std::variant<FIXMessage *, FIXSessionMessage*, BIST::ITCHMessage, NASDAQ::ITCHMessage, BIST::OUCHMessage, NASDAQ::OUCHMessage>; 
using spscMessageQueue_t = boost::lockfree::spsc_queue<MessageWithVenue<MessageTypes_t>, boost::lockfree::capacity<MESSAGE_QUEUE_CAPACITY>>;
using spscFIXOutSessionQueue_t = boost::lockfree::spsc_queue<FIXSessionMessage*, boost::lockfree::capacity<MESSAGE_QUEUE_CAPACITY>>;
using spscDbQueue_t = boost::lockfree::spsc_queue<std::variant<Order *, MessageWithVenue<FIXMessage *>, MessageWithVenue<FIXSessionMessage *>, MessageWithVenue<BIST::ITCHMessage>, MessageWithVenue<BIST::OUCHMessage>, MessageWithVenue<NASDAQ::ITCHMessage>, MessageWithVenue<NASDAQ::OUCHMessage>>, boost::lockfree::capacity<DB_QUEUE_CAPACITY>>;
using DbData_t = std::variant<Order *, MessageWithVenue<FIXMessage *>, MessageWithVenue<FIXSessionMessage *>, MessageWithVenue<BIST::ITCHMessage>, MessageWithVenue<BIST::OUCHMessage>, MessageWithVenue<NASDAQ::ITCHMessage>, MessageWithVenue<NASDAQ::OUCHMessage>>;

class Parser_Dispatch
{
private:
    static constexpr uint16_t DB_QUEUE_THRESHOLD = 256;
    static inline uint16_t PktCount = 1;

    Parser_FIX fixparser_{sess_mngr_, parser_to_fixbuilder_in_};
    Parser_ITCH_BIST itchparser_bist_;
    Parser_ITCH_NASDAQ itchparser_nasdaq_;
    Parser_OUCH_BIST ouchparser_bist_;
    Parser_OUCH_NASDAQ ouchparser_nasdaq_;
    
    using ParserFunc = void (Parser_Dispatch::*)(OutPacket*) noexcept;
    std::array<std::array<ParserFunc, VENUE_COUNT>, PROTOCOL_COUNT> makeParserLookUpTable() noexcept; 
    std::array<std::array<ParserFunc, VENUE_COUNT>, PROTOCOL_COUNT> parser_table_;

    spscPacketQueue_t &receiver_to_parser_;
    spscMessageQueue_t &parser_to_store_;
    spscFIXOutSessionQueue_t &parser_to_fixbuilder_out_;
    spscFIXInSessionQueue_t &parser_to_fixbuilder_in_;
    SessionManager &sess_mngr_;
    spscDbQueue_t &db_to_parser_;
    NetworkIO &network_io_;

public:
    Parser_Dispatch(spscPacketQueue_t &receiver_to_parser, spscMessageQueue_t &parser_to_store, spscFIXOutSessionQueue_t &parser_to_fixbuilder_out, spscFIXInSessionQueue_t &parser_to_fixbuilder_in, SessionManager &sess_mngr, spscDbQueue_t &db_to_parser, NetworkIO &network_io) noexcept;

    bool dispatch() noexcept;

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
                if constexpr (std::is_same_v<T, MessageWithVenue<FIXSessionMessage *>>)
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
                else if constexpr (std::is_same_v<T, MessageWithVenue<NASDAQ::OUCHMessage>>)
                {
                    ouchparser_nasdaq_.releaseOUCH(item.msg);
                }
                else if constexpr (std::is_same_v<T, Order*>)
                {
                    return;
                }
            
            }, DbQueueItem);   
        }
    }

    inline void proceedPendingFIX(auto *variant_msg, auto& seq_fix, SessionState& state) noexcept
    {
        std::visit([&](auto *msg)
                   {
                       using MsgType = std::remove_pointer_t<decltype(msg)>;

                       if constexpr (std::is_same_v<MsgType, FIXSessionMessage>)
                       {
                           if (msg->msg_type != static_cast<uint8_t>(FIXTypes::Logon) && !fixparser_.handle_sesMsg(msg, seq_fix, state))
                               parser_to_fixbuilder_out_.push(msg);
                       }

                       parser_to_store_.push(MessageWithVenue<MessageTypes_t>(msg, Venue::BIST));
                       fixparser_.pop_pending(msg->seqnum);
                   },
                   *variant_msg);
    }

    inline void parseFIX(OutPacket* pkt) noexcept
    {
        uint8_t sess_index = sess_mngr_.getSessionIndex(pkt->venue, pkt->protocol);
        SessionState* state = sess_mngr_.getSessionState(sess_index);
        Sequence_FIX& seq_fix = state->fix;
        
        size_t offset = 0;

        while (true)
        {
            auto pair = fixparser_.nextFixMsg(pkt, offset);
            if (!pair.first)
                break;

            using V_t = std::variant<FIXMessage*, FIXSessionMessage*>;
            FIXMessage* fixMsg{nullptr};
            FIXSessionMessage* fixSesMsg{nullptr};
            const auto type = fixparser_.find_type(pair.first);
            
            if(type > 65) 
            {
                fixMsg = fixparser_.parse<FIXMessage>(pair.first, pair.second);
                if (seq_fix.get_expected() == fixMsg->seqnum)
                {
                    parser_to_store_.push(MessageWithVenue<MessageTypes_t>(fixMsg, Venue::BIST));
                    seq_fix.increase_expected_seq();

                    while (V_t *variant_msg = fixparser_.find_in_pending(seq_fix.get_expected()))
                    {
                        proceedPendingFIX(variant_msg, seq_fix, *state);
                        seq_fix.increase_expected_seq();
                    }
                }
                else
                {
                    fixparser_.resend_logic(fixMsg->seqnum, seq_fix);
                    fixparser_.push_pending(fixMsg);
                }
            }
            else
            {
                fixSesMsg = fixparser_.parse<FIXSessionMessage>(pair.first, pair.second);
                if (seq_fix.get_expected() == fixSesMsg->seqnum)
                {
                    if (!fixparser_.handle_sesMsg(fixSesMsg, seq_fix, *state))
                        parser_to_fixbuilder_out_.push(fixSesMsg);

                    seq_fix.increase_expected_seq();

                    while (V_t *variant_msg = fixparser_.find_in_pending(seq_fix.get_expected()))
                    {
                        proceedPendingFIX(variant_msg, seq_fix, *state);
                        seq_fix.increase_expected_seq();
                    }
                }
                else
                {
                    if (LIKELY(fixSesMsg->msg_type != static_cast<uint8_t>(FIXTypes::Logon))) 
                    {
                        fixparser_.resend_logic(fixSesMsg->seqnum, seq_fix);
                        fixparser_.push_pending(fixSesMsg);
                    }
                    else
                    {
                        fixparser_.resend_logic_logon(fixSesMsg, seq_fix);
                    }
                }

                parser_to_store_.push(MessageWithVenue<MessageTypes_t>(fixSesMsg, Venue::BIST));
            }
            offset += pair.second;
        }
    }

    inline void parseITCH_BIST(OutPacket *pkt) noexcept
    {
        BIST::ITCHMessage itchMsg{itchparser_bist_.parse(pkt->data.data())};
        parser_to_store_.push(MessageWithVenue<MessageTypes_t>(itchMsg, Venue::BIST));
    }

    inline void parseITCH_NASDAQ(OutPacket *pkt) noexcept
    {
        NASDAQ::ITCHMessage itchMsg{itchparser_nasdaq_.parse(pkt->data.data())};
        parser_to_store_.push(MessageWithVenue<MessageTypes_t>(itchMsg, Venue::NASDAQ));
    }

    inline void parseOUCH_BIST(OutPacket *pkt) noexcept
    {
        for(uint8_t msg_index = 0; msg_index < pkt->msg_count; msg_index) 
        {
            BIST::OUCHMessage ouchMsg{ouchparser_bist_.parse(pkt->data.data() + pkt->offsets[msg_index])};
            parser_to_store_.push(MessageWithVenue<MessageTypes_t>(ouchMsg, Venue::BIST));
        }
    }

    inline void parseOUCH_NASDAQ(OutPacket *pkt) noexcept
    {
        for (uint8_t msg_index = 0; msg_index < pkt->msg_count; msg_index)
        {
            NASDAQ::OUCHMessage ouchMsg{ouchparser_nasdaq_.parse(pkt->data.data() + pkt->offsets[msg_index])};
            parser_to_store_.push(MessageWithVenue<MessageTypes_t>(ouchMsg, Venue::NASDAQ));
        }
    }
    
};
