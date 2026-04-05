#include "Parser_Dispatch.h"
#include "NetworkIO.h"

#include <type_traits>

Parser_Dispatch::Parser_Dispatch(spscOutPacketQueue_t &receiver_to_parser, spscMessageQueue_t &parser_to_store, spscFIXOutSessionQueue_t &parser_to_fixbuilder_out, spscFIXInSessionQueue_t &parser_to_fixbuilder_in,
                                 SessionManager &sess_mngr, spscDbQueue_t &db_to_parser, NetworkIO &network_io) noexcept
                                 : parser_table_(makeParserLookUpTable()), receiver_to_parser_(receiver_to_parser), parser_to_store_(parser_to_store), parser_to_fixbuilder_out_(parser_to_fixbuilder_out), 
                                 parser_to_fixbuilder_in_(parser_to_fixbuilder_in), sess_mngr_(sess_mngr), db_to_parser_(db_to_parser), network_io_(network_io)
{}

std::array<std::array<Parser_Dispatch::ParserFunc, VENUE_COUNT>, PROTOCOL_COUNT> Parser_Dispatch::makeParserLookUpTable() noexcept
{
   std::array<std::array<Parser_Dispatch::ParserFunc, VENUE_COUNT>, PROTOCOL_COUNT> parser_table{};

   parser_table[static_cast<size_t>(Protocol::FIX)][static_cast<size_t>(Venue::BIST)] = &Parser_Dispatch::parseFIX;

   parser_table[static_cast<size_t>(Protocol::ITCH)][static_cast<size_t>(Venue::BIST)] = &Parser_Dispatch::parseITCH_BIST;
   parser_table[static_cast<size_t>(Protocol::ITCH)][static_cast<size_t>(Venue::NASDAQ)] = &Parser_Dispatch::parseITCH_NASDAQ;

   parser_table[static_cast<size_t>(Protocol::OUCH)][static_cast<size_t>(Venue::BIST)] = &Parser_Dispatch::parseOUCH_BIST;
   parser_table[static_cast<size_t>(Protocol::OUCH)][static_cast<size_t>(Venue::NASDAQ)] = &Parser_Dispatch::parseOUCH_NASDAQ;

   return parser_table;
}

bool Parser_Dispatch::dispatch() noexcept
{
   OutPacket *pkt = nullptr;
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
   
   (this->*parser_table_[static_cast<size_t>(pkt->protocol)][static_cast<size_t>(pkt->venue)])(pkt);
   if(pkt->release_this_pkt)
      network_io_.releasePacket(pkt);

   return true;
}

void Parser_Dispatch::flush_DbQueue() noexcept
{
   while (!db_to_parser_.empty())
   {
      DbData_t DbQueueItem;
      db_to_parser_.pop(DbQueueItem);
      std::visit([&](auto &item)
                 {
                    using T = std::decay_t<decltype(item)>;
                    if constexpr (std::is_same_v<T, MessageWithVenue<FIXMessage *>>)
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
                    else if constexpr (std::is_same_v<T, Order *>)
                    {
                       return;
                    }
                 },
                 DbQueueItem);
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
                               parser_to_fixbuilder_out_.push(msg);
                       }
                       else
                       { 
                           parser_to_store_.push(MessageWithVenue<MessageTypes_t>(msg, Venue::BIST));
                       }

                       seqnum = msg->seqnum; 
               }, 
               *variant_msg);

   fixparser_.pop_pending(seqnum);
}

void Parser_Dispatch::parseFIX(OutPacket *pkt) noexcept
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

   //-----------------------------------DEBUG_ONLY-----------------------------------------------
   /* IF_NOT_RELEASE (
      if(fixMsg == nullptr) 
      {
         parser_to_store_.push(MessageWithVenue<MessageTypes_t>(fixMsg, Venue::BIST));
         seq_fix.increase_expected_seq();
         offset += pair.second;
         return;
      }
   ); */
   //--------------------------------------------------------------------------------------------
         
            if (seq_fix.get_expected() == fixMsg->seqnum)   
            {
               parser_to_store_.push(MessageWithVenue<MessageTypes_t>(fixMsg, Venue::BIST));
               seq_fix.increase_expected_seq();
               
               while(V_t* variant_msg = fixparser_.find_in_pending(seq_fix.get_expected()))
               {
                  proceedPendingFIX(variant_msg, seq_fix, *state);
                  seq_fix.increase_expected_seq();
               }
            }
            else if(seq_fix.get_expected() < fixMsg->seqnum)
            {
               fixparser_.resend_logic(fixMsg->seqnum, seq_fix);
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
                  parser_to_fixbuilder_out_.push(fixSesMsg);

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

            parser_to_store_.push(MessageWithVenue<MessageTypes_t>(fixSesMsg, Venue::BIST));
         }
      }

      if(!pkt->consumed)
         continue;

      break;
   }
}

void Parser_Dispatch::parseITCH_BIST(OutPacket *pkt) noexcept
{
   BIST::ITCHMessage itchMsg{itchparser_bist_.parse(pkt->data.data())};
   parser_to_store_.push(MessageWithVenue<MessageTypes_t>(itchMsg, Venue::BIST));
}

void Parser_Dispatch::parseITCH_NASDAQ(OutPacket *pkt) noexcept
{
   NASDAQ::ITCHMessage itchMsg{itchparser_nasdaq_.parse(pkt->data.data())};
   parser_to_store_.push(MessageWithVenue<MessageTypes_t>(itchMsg, Venue::NASDAQ));
}

void Parser_Dispatch::parseOUCH_BIST(OutPacket *pkt) noexcept
{
   for (uint8_t msg_index = 0; msg_index < pkt->msg_count; ++msg_index)
   {
      BIST::OUCHMessage ouchMsg{ouchparser_bist_.parse(pkt->data.data() + pkt->offsets[msg_index])};
      parser_to_store_.push(MessageWithVenue<MessageTypes_t>(ouchMsg, Venue::BIST));
   }
}

void Parser_Dispatch::parseOUCH_NASDAQ(OutPacket *pkt) noexcept
{
   for (uint8_t msg_index = 0; msg_index < pkt->msg_count; ++msg_index)
   {
      NASDAQ::OUCHMessage ouchMsg{ouchparser_nasdaq_.parse(pkt->data.data() + pkt->offsets[msg_index])};
      parser_to_store_.push(MessageWithVenue<MessageTypes_t>(ouchMsg, Venue::NASDAQ));
   }
}