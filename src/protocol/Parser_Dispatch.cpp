#include "Parser_Dispatch.h"
#include "NetworkIO.h"
#include "Parser_FIX.h"
#include "SessionManager.h"

#include <functional>
#include <type_traits>

Parser_Dispatch::Parser_Dispatch(spscRxPacketQueue_t &receiver_to_parser, spscMessageQueue_t &parser_to_store, spscFIXRxSessionQueue_t &parser_to_fixbuilder_rx, spscFIXTxSessionQueue_t &parser_to_fixbuilder_tx,
                                 SessionManager &sess_mngr, spscDbQueue_t &db_to_parser, NetworkIO &network_io, Parser_FIX& fixparser, MWV_PoolManager_t &mwv_poolmanager) noexcept
                                 : receiver_to_parser_(receiver_to_parser), parser_to_store_(parser_to_store), parser_to_fixbuilder_rx_(parser_to_fixbuilder_rx), 
                                 parser_to_fixbuilder_tx_(parser_to_fixbuilder_tx), sess_mngr_(sess_mngr), db_to_parser_(db_to_parser), network_io_(network_io), fixparser_(fixparser), mwv_poolmanager_(mwv_poolmanager)
{}

std::array<std::array<Parser_Dispatch::ParserFunc, VENUE_COUNT>, PROTOCOL_COUNT> Parser_Dispatch::makeParserLookUpTable() noexcept
{
   std::array<std::array<Parser_Dispatch::ParserFunc, VENUE_COUNT>, PROTOCOL_COUNT> parser_table{};

   parser_table[to_underlying(Protocol::FIX)][to_underlying(Venue::BIST)] = &Parser_Dispatch::parseFIX;

   parser_table[to_underlying(Protocol::ITCH)][to_underlying(Venue::BIST)] = &Parser_Dispatch::parseITCH_BIST;
   parser_table[to_underlying(Protocol::ITCH)][to_underlying(Venue::NASDAQ)] = &Parser_Dispatch::parseITCH_NASDAQ;

   parser_table[to_underlying(Protocol::OUCH)][to_underlying(Venue::BIST)] = &Parser_Dispatch::parseOUCH_BIST;
   parser_table[to_underlying(Protocol::OUCH)][to_underlying(Venue::NASDAQ)] = &Parser_Dispatch::parseOUCH_NASDAQ;

   return parser_table;
}

std::array<std::array<Parser_Dispatch::ParserFunc, VENUE_COUNT>, PROTOCOL_COUNT> Parser_Dispatch::parser_table_ = makeParserLookUpTable();

bool Parser_Dispatch::dispatch() 
{
   RxPacket *pkt = nullptr;
   receiver_to_parser_.pop(pkt);

   if(!pkt)
   {
      return false;
   }
   else if (pkt->len <= 0)
   {
      network_io_.releasePacket(pkt);
      return true;
   }

   
   if (UNLIKELY((PktCount++ & (DB_QUEUE_THRESHOLD - 1u)) == 0))
   {
      flush_DbQueue();
      PktCount = 1;
   }

   std::invoke(parser_table_[to_underlying(pkt->protocol)][to_underlying(pkt->venue)], this, pkt);
   
   if(pkt->release_this_pkt)
      network_io_.releasePacket(pkt);


   return true;
}

void Parser_Dispatch::flush_DbQueue() noexcept
{
   DbData* DbQueueItem;

   while (db_to_parser_.pop(DbQueueItem))
   {   
      std::visit([&](auto item)
                 {
                    using T = std::remove_pointer_t<std::decay_t<decltype(item)>>;
                    if constexpr (std::is_same_v<T, MessageWithVenue<FIXMessage *>>)
                    {
                       fixparser_.releaseFIX(item->msg);
                    }
                    else if constexpr (std::is_same_v<T, MessageWithVenue<FIXSessionMessage *>>)
                    {
                       fixparser_.releaseFIX(item->msg);
                    }
                    else if constexpr (std::is_same_v<T, MessageWithVenue<BIST::ITCHMessage>>)
                    {
                       itchparser_bist_.releaseITCH(item->msg);
                    }
                    else if constexpr (std::is_same_v<T, MessageWithVenue<BIST::OUCHMessage>>)
                    {
                       ouchparser_bist_.releaseOUCH(item->msg);
                    }
                    else if constexpr (std::is_same_v<T, MessageWithVenue<NASDAQ::ITCHMessage>>)
                    {
                       itchparser_nasdaq_.releaseITCH(item->msg);
                    }
                    else if constexpr (std::is_same_v<T, MessageWithVenue<NASDAQ::OUCHMessage>>)
                    {
                       ouchparser_nasdaq_.releaseOUCH(item->msg);
                    }
                    else if constexpr (std::is_same_v<T, Order>)
                    {
                       assert(false && "Order must not reach this visitor");
                    }
                 },
                 DbQueueItem->data);
   }
}

void Parser_Dispatch::proceedPendingFIX(auto* variant_msg, auto& seq_fix, SessionState& state) noexcept
{
   uint32_t seqnum;
   std::visit([&](auto* msg)
              {         
                       using MsgType = std::remove_pointer_t<decltype(msg)>;
                       
                       if constexpr (std::is_same_v<MsgType, FIXSessionMessage>)
                       {
                           if(msg->msg_type != static_cast<uint8_t>(FIXTypes::Logon) && !fixparser_.handle_sesMsg(msg, seq_fix, state))
                               parser_to_fixbuilder_rx_.push(msg);
                       }
                       else
                       { 
                           parser_to_store_.push(mwv_poolmanager_.get_msgWithVenue<MessageTypes_t>(msg, Venue::BIST));
                           // fixparser_.releaseFIX(msg);   
                       }

                       seqnum = msg->seqnum; 
               }, 
               *variant_msg);

   fixparser_.pop_pending(seqnum);
}

void Parser_Dispatch::parseFIX(RxPacket *pkt) 
{
   uint8_t sess_index = sess_mngr_.getSessionIndex(pkt->venue, pkt->protocol);
   SessionState *state = sess_mngr_.getSessionState(sess_index);
   Sequence_FIX &seq_fix = state->fix;

   size_t offset = 0;

   while (true)
   {
      auto pair = fixparser_.nextFixMsg(pkt, offset);

      using V_t = std::variant<FIXMessage*, FIXSessionMessage*>;
      
      if(pair.first)
      {
         const auto type =  fixparser_.find_type(pair.first); 
         
         if (type >= 56 && type != 'A')
         {
            FIXMessage *fixMsg{nullptr};

            fixMsg = fixparser_.parse<FIXMessage>(pair.first, pair.second);

            if (seq_fix.get_expected() == fixMsg->seqnum)   
            {
                  
               parser_to_store_.push(mwv_poolmanager_.get_msgWithVenue<MessageTypes_t>(fixMsg, Venue::BIST));
               // fixparser_.releaseFIX(fixMsg);               
               seq_fix.increase_expected_seq();
               
               while(V_t* variant_msg = fixparser_.find_in_pending(seq_fix.get_expected()))
               {
                  proceedPendingFIX(variant_msg, seq_fix, *state);
                  seq_fix.increase_expected_seq();
               }
            }
            else if(seq_fix.get_expected() < fixMsg->seqnum)
            {
               // fixparser_.resend_logic(fixMsg->seqnum, seq_fix);
               fixparser_.push_pending(fixMsg);
            }
         }
         else
         {
            FIXSessionMessage *fixSesMsg{nullptr};

            fixSesMsg = fixparser_.parse<FIXSessionMessage>(pair.first, pair.second);
            if (seq_fix.get_expected() == fixSesMsg->seqnum)
            {
               if (!fixparser_.handle_sesMsg(fixSesMsg, seq_fix, *state))
                  parser_to_fixbuilder_rx_.push(fixSesMsg);

               seq_fix.increase_expected_seq();

               while(V_t* variant_msg = fixparser_.find_in_pending(seq_fix.get_expected()))
               {
                  proceedPendingFIX(variant_msg, seq_fix, *state);
                  seq_fix.increase_expected_seq();
               }
            }
            else if(seq_fix.get_expected() < fixSesMsg->seqnum)
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

            parser_to_store_.push(mwv_poolmanager_.get_msgWithVenue<MessageTypes_t>(fixSesMsg, Venue::BIST));
         }
      }

      if(!pkt->consumed)
         continue;

      break;
   }
}

void Parser_Dispatch::parseITCH_BIST(RxPacket *pkt) noexcept
{
   itchparser_bist_.ItchPacketHandler(pkt);

   for (size_t msg_index = 0; msg_index < pkt->msg_count; ++msg_index)
   {
      BIST::ITCHMessage itchMsg{itchparser_bist_.parse(pkt->data.data() + pkt->offsets[msg_index])};
      parser_to_store_.push(mwv_poolmanager_.get_msgWithVenue<MessageTypes_t>(itchMsg, Venue::BIST));

      // itchparser_bist_.releaseITCH(itchMsg);
   }
}

void Parser_Dispatch::parseITCH_NASDAQ(RxPacket *pkt) noexcept
{
   itchparser_nasdaq_.ItchPacketHandler(pkt);

   for (size_t msg_index = 0; msg_index < pkt->msg_count; ++msg_index)
   {
      NASDAQ::ITCHMessage itchMsg{itchparser_nasdaq_.parse(pkt->data.data() + pkt->offsets[msg_index])};
      parser_to_store_.push(mwv_poolmanager_.get_msgWithVenue<MessageTypes_t>(itchMsg, Venue::NASDAQ));
      
      // itchparser_nasdaq_.releaseITCH(itchMsg);
   }

   // BATCH START
      // std::array<MessageWithVenue<MessageTypes_t>, 3> batch;
      // size_t count = 0;

      // for (size_t i = 0; i < pkt->msg_count; ++i)
      //    batch[count++] = {itchparser_nasdaq_.parse(pkt->data.data() + pkt->offsets[i]), Venue::NASDAQ};

      // for (size_t i = 0; i < count; ++i)
      //    parser_to_store_.push(batch[i]);
   // BATCH END
}

void Parser_Dispatch::parseOUCH_BIST(RxPacket *pkt) noexcept
{
   for (size_t msg_index = 0; msg_index < pkt->msg_count; ++msg_index)
   {
      BIST::OUCHMessage ouchMsg{ouchparser_bist_.parse(pkt->data.data() + pkt->offsets[msg_index])};
      parser_to_store_.push(mwv_poolmanager_.get_msgWithVenue<MessageTypes_t>(ouchMsg, Venue::BIST));

      // ouchparser_bist_.releaseOUCH(ouchMsg);
   }

}

void Parser_Dispatch::parseOUCH_NASDAQ(RxPacket *pkt) noexcept
{
   for (size_t msg_index = 0; msg_index < pkt->msg_count; ++msg_index)
   {
      NASDAQ::OUCHMessage ouchMsg{ouchparser_nasdaq_.parse(pkt->data.data() + pkt->offsets[msg_index])};
      parser_to_store_.push(mwv_poolmanager_.get_msgWithVenue<MessageTypes_t>(ouchMsg, Venue::NASDAQ));
      
      // ouchparser_nasdaq_.releaseOUCH(ouchMsg);
   }
}