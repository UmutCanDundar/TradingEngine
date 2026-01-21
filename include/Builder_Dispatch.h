#pragma once

#include "Builder_FIX.h"
#include "Builder_OUCH_BIST.h"
#include "Builder_OUCH_NASDAQ.h"
#include "RiskEngine.h"
#include "Parser_Dispatch.h"
#include "SoupBinTcp.h"
#include "SessionManager.h"

#include <variant>
#include <boost/lockfree/spsc_queue.hpp>
#include <absl/container/btree_map.h>

class SoupBinTcp;

class Builder_Dispatch
{
private:
  
    Builder_FIX fixBuilder_{sess_mngr_};
    Builder_OUCH_BIST ouchBuilder_bist_{sbt_};
    Builder_OUCH_NASDAQ ouchBuilder_nasdaq_{sbt_};
   
    using BuilderFunc = void (Builder_Dispatch::*)(Order*) noexcept;
    std::array<std::array<BuilderFunc, VENUE_COUNT>, PROTOCOL_COUNT> makeBuilderLookUpTable() noexcept; 
    std::array<std::array<BuilderFunc, VENUE_COUNT>, PROTOCOL_COUNT> builder_table_;
    
    spscFIXInSessionQueue_t &parser_to_fixbuilder_in_;
    spscFIXOutSessionQueue_t &parser_to_fixbuilder_out_;
    spscOrderQueue_t &risk_to_builder_;
    spscInPacketPayloadQueue_t &builder_to_sender_;
    SoupBinTcp &sbt_;
    SessionManager& sess_mngr_;

public:
    Builder_Dispatch(spscInPacketPayloadQueue_t &builder_to_sender, spscOrderQueue_t &risk_to_builder, spscFIXOutSessionQueue_t &parser_to_fixbuilder_out, spscFIXInSessionQueue_t &parser_to_fixbuilder_in, SessionManager& sess_mngr, SoupBinTcp &sbt) noexcept;

    bool dispatch() noexcept;

private:
    inline void buildFIX_ses_in(FIXSessionMessage &fixSesMsg, uint8_t session_index) noexcept
    {
        Buffer_FIX *buffer;

        switch (static_cast<FIXTypes>(fixSesMsg.msg_type))
        {
            case FIXTypes::ResendRequest:
                buffer = fixBuilder_.build<FIXTypes::ResendRequest>(session_index, fixSesMsg.begin_seqnum, fixSesMsg.end_seqnum);
                break;
            
            case FIXTypes::Logout:
                buffer = fixBuilder_.build<FIXTypes::Logout>(session_index);
                break;
            
            case FIXTypes::Logon:
                buffer = fixBuilder_.build<FIXTypes::Logon>(session_index, false);
                break;
            
            default:
                return;
        }

        builder_to_sender_.push({buffer->data, buffer->len, session_index});
    }

    inline void buildFIX_ses_out(FIXSessionMessage &fixSesMsg, uint8_t session_index) noexcept
    {
        Buffer_FIX *buffer;

        switch (static_cast<FIXTypes>(fixSesMsg.msg_type))
        {
            case FIXTypes::TestRequest:
                buffer = fixBuilder_.build<FIXTypes::Heartbeat>(session_index);
                break;

            case FIXTypes::ResendRequest:
            {
                auto begin = fixSesMsg.begin_seqnum;
                auto end = fixSesMsg.end_seqnum; 
                for (auto i = begin; i > end; i++)
                { 
                    auto *buffer = fixBuilder_.build_resend(sess_mngr_.getSessionState(session_index)->fix.get_buffer(i), session_index);
                    builder_to_sender_.push({buffer->data, buffer->len, session_index});
                }
                return;
            }

            case FIXTypes::Reject:
                buffer = fixBuilder_.build<FIXTypes::Logout>(session_index);
                break;

            default:
                return;
        }

        builder_to_sender_.push({buffer->data, buffer->len, session_index});
    }

    inline void buildFIX_app(Order* order, uint8_t session_index) noexcept
    {
        Buffer_FIX* buffer;

        switch (static_cast<FIXTypes>(order->message_type))
        {
            case FIXTypes::NewOrderSingle:
                buffer = fixBuilder_.build<FIXTypes::NewOrderSingle>(session_index, *order);
                break;
            
            case FIXTypes::OrderCancelRequest:
                buffer = fixBuilder_.build<FIXTypes::OrderCancelRequest>(session_index, *order);
                break;
            
            case FIXTypes::OrderCancelReplaceRequest:
                buffer = fixBuilder_.build<FIXTypes::OrderCancelReplaceRequest>(session_index, *order);
                break;
            default:
                return;
        }

        builder_to_sender_.push({buffer->data, buffer->len, session_index});
        order->canModify.store(order->canModify | BUILDER_DONE, std::memory_order_relaxed);
    }

    inline void buildFIX(Order *order) noexcept
    {
        FIXSessionMessage *fixSesMsg{nullptr};
        uint8_t session_index = sess_mngr_.getSessionIndex(Venue::BIST, Protocol::FIX);

        while (parser_to_fixbuilder_in_.pop(fixSesMsg)) 
            buildFIX_ses_in(*fixSesMsg, session_index);
        while (parser_to_fixbuilder_out_.pop(fixSesMsg))
            buildFIX_ses_out(*fixSesMsg, session_index);

        buildFIX_app(order, session_index);
        order->canModify.store(order->canModify | BUILDER_DONE, std::memory_order_relaxed);
    }

    inline void buildOUCH_BIST(Order *order) noexcept {
        uint8_t session_index = sess_mngr_.getSessionIndex(Venue::BIST, Protocol::OUCH);
        auto* buffer = ouchBuilder_bist_.build(*order); 
        auto bufWithVenue = InPacketPayload{buffer->msg, buffer->len, session_index};
        builder_to_sender_.push(bufWithVenue);
        order->canModify.store(order->canModify | BUILDER_DONE, std::memory_order_relaxed);
    }

    inline void buildOUCH_NASDAQ(Order* order) noexcept
    {
        uint8_t session_index = sess_mngr_.getSessionIndex(Venue::NASDAQ, Protocol::OUCH);
        auto *buffer = ouchBuilder_nasdaq_.build(*order);
        auto bufWithVenue = InPacketPayload{buffer->msg, buffer->len, session_index};
        builder_to_sender_.push(bufWithVenue);
        order->canModify.store(order->canModify | BUILDER_DONE, std::memory_order_relaxed);
    }

};
