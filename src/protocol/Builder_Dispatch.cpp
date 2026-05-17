#include "Builder_Dispatch.h"
#include "Parser_FIX.h"
#include "Builder_FIX.h"
#include "SessionManager.h"
#include "LoginController.h"
#include "OrderManager.h"
#include "Logger.h"

#include <atomic>
#include <cstddef>

Builder_Dispatch::Builder_Dispatch(spscTxPacketQueue_t &builder_to_sender, spscOrderQueue_t &risk_to_builder, spscFIXOutSessionQueue_t &parser_to_fixbuilder_out, spscFIXInSessionQueue_t &parser_to_fixbuilder_in,
                                   SessionManager &sess_mngr, SoupBinTcp &sbt, LoginController &login, TxPacketPoolManager &txPkt_pool, Builder_FIX &fixBuilder, OrderManager &ord_mngr, Parser_FIX &fixParser) noexcept
                                   : builder_table_(makeBuilderLookUpTable()), builder_to_sender_(builder_to_sender), risk_to_builder_(risk_to_builder), parser_to_fixbuilder_out_(parser_to_fixbuilder_out), 
                                   parser_to_fixbuilder_in_(parser_to_fixbuilder_in), sess_mngr_(sess_mngr), sbt_(sbt), login_(login), txPkt_pool_(txPkt_pool), fixBuilder_(fixBuilder), ord_mngr_(ord_mngr),
                                   fixParser_(fixParser)
{}

std::array<std::array<Builder_Dispatch::BuilderFunc, VENUE_COUNT>, PROTOCOL_COUNT> Builder_Dispatch::makeBuilderLookUpTable() noexcept
{
   std::array<std::array<Builder_Dispatch::BuilderFunc, VENUE_COUNT>, PROTOCOL_COUNT> Builder_table{};  // 3/8 being used

   Builder_table[static_cast<size_t>(Protocol::FIX)][static_cast<size_t>(Venue::BIST)] = &Builder_Dispatch::buildFIX;

   Builder_table[static_cast<size_t>(Protocol::OUCH)][static_cast<size_t>(Venue::BIST)] = &Builder_Dispatch::buildOUCH_BIST;
   Builder_table[static_cast<size_t>(Protocol::OUCH)][static_cast<size_t>(Venue::NASDAQ)] = &Builder_Dispatch::buildOUCH_NASDAQ;

   return Builder_table;
}

bool Builder_Dispatch::dispatch() noexcept
{
   Order* order{nullptr};

   if(!risk_to_builder_.pop(order))
      return false;

   (this->*builder_table_[static_cast<size_t>(order->protocol)][static_cast<size_t>(order->venue)])(order);

   if(order->order_id == 0) 
      ord_mngr_.add_awaitingAck_order(*order);
   
   return true;
}

void Builder_Dispatch::buildFIX(Order *order) noexcept
{
   FIXSessionMessage *fixSesMsg{nullptr};
   uint8_t session_index = sess_mngr_.getSessionIndex(Venue::BIST, Protocol::FIX);

   while (parser_to_fixbuilder_out_.pop(fixSesMsg))
      buildFIX_ses_out(*fixSesMsg, session_index);

   while (parser_to_fixbuilder_in_.pop(fixSesMsg))
      buildFIX_ses_in(*fixSesMsg, session_index);

   buildFIX_app(order, session_index);
   order->canModify.fetch_or(BUILDER_DONE, std::memory_order_release);
}

void Builder_Dispatch::buildOUCH_BIST(Order *order) noexcept 
{
   uint8_t session_index = sess_mngr_.getSessionIndex(Venue::BIST, Protocol::OUCH); 
   auto* buffer = ouchBuilder_bist_.build(*order);
   TxPacket *txPkt = txPkt_pool_.get_txPkt();
   txPkt->fillPacket(buffer->msg, buffer->len, session_index);
   builder_to_sender_.push(txPkt);
   order->canModify.fetch_or(BUILDER_DONE, std::memory_order_release);
}

void Builder_Dispatch::buildOUCH_NASDAQ(Order *order) noexcept
{
   uint8_t session_index = sess_mngr_.getSessionIndex(Venue::NASDAQ, Protocol::OUCH);
   auto *buffer = ouchBuilder_nasdaq_.build(*order);
   TxPacket *txPkt = txPkt_pool_.get_txPkt();
   txPkt->fillPacket(buffer->msg, buffer->len, session_index);
   builder_to_sender_.push(txPkt);
   order->canModify.fetch_or(BUILDER_DONE, std::memory_order_release);
}

void Builder_Dispatch::buildFIX_ses_in(FIXSessionMessage &fixSesMsg, uint8_t session_index) noexcept
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
   {
      if (!sess_mngr_.isSessionLoggedInBefore(session_index))
         buffer = fixBuilder_.build<FIXTypes::Logon>(session_index, true);
      else
         buffer = fixBuilder_.build<FIXTypes::Logon>(session_index, false);

      TxPacket *txPkt = txPkt_pool_.get_txPkt();
      txPkt->fillPacket(buffer->data, buffer->len, session_index, true);
      builder_to_sender_.push(txPkt);
      fixParser_.releaseFIX(&fixSesMsg);
      return;
   }
   default:
      fixParser_.releaseFIX(&fixSesMsg);
      return;
   }

   TxPacket *txPkt = txPkt_pool_.get_txPkt();
   txPkt->fillPacket(buffer->data, buffer->len, session_index);
   builder_to_sender_.push(txPkt);
   fixParser_.releaseFIX(&fixSesMsg);
}

void Builder_Dispatch::buildFIX_ses_out(FIXSessionMessage &fixSesMsg, uint8_t session_index) noexcept
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
         TxPacket *txPkt = txPkt_pool_.get_txPkt();
         txPkt->fillPacket(buffer->data, buffer->len, session_index);
         builder_to_sender_.push(txPkt);
      }
      fixParser_.releaseFIX(&fixSesMsg);
      return;
   }
   case FIXTypes::Reject:
      buffer = fixBuilder_.build<FIXTypes::Logout>(session_index);
      break;

   case FIXTypes::Logout:
   {
      auto *state = sess_mngr_.getSessionState(session_index);
      LoginDecision login_decision;
      login_decision = login_.LoginDecider(state);
      if (login_decision == LoginDecision::TryAgain)
      {
         if (!sess_mngr_.isSessionLoggedInBefore(session_index))
            buffer = fixBuilder_.build<FIXTypes::Logon>(session_index, true);
         else
            buffer = fixBuilder_.build<FIXTypes::Logon>(session_index, false);

         state->last_login_attempt_ns = LoginController::NowNs();
         ++state->login_retry_count;

         TxPacket *txPkt = txPkt_pool_.get_txPkt();
         txPkt->fillPacket(buffer->data, buffer->len, session_index, true);
         builder_to_sender_.push(txPkt);
         fixParser_.releaseFIX(&fixSesMsg);
         return;
      }
      else
      {
         LOG_ERROR("Login failed after {} attempts for session {}",
                   ALLOWED_LOGIN_ATTEMPT, session_index);
         fixParser_.releaseFIX(&fixSesMsg);
         return;
      }

      break;
   }
   default:
      fixParser_.releaseFIX(&fixSesMsg);
      return;
   }

   TxPacket *txPkt = txPkt_pool_.get_txPkt();
   txPkt->fillPacket(buffer->data, buffer->len, session_index);
   builder_to_sender_.push(txPkt);
   fixParser_.releaseFIX(&fixSesMsg);
}

void Builder_Dispatch::buildFIX_app(Order *order, uint8_t session_index) noexcept
{
   Buffer_FIX *buffer;

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

   TxPacket *txPkt = txPkt_pool_.get_txPkt();
   txPkt->fillPacket(buffer->data, buffer->len, session_index);
   builder_to_sender_.push(txPkt);
}



