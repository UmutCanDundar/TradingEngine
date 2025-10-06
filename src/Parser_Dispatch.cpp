#include "Parser_Dispatch.h"

Parser_Dispatch::Parser_Dispatch(spscPacketQueue_t &receiver_to_parser, spscMessageQueue_t &parser_to_store)
    : receiver_to_parser_(receiver_to_parser), parser_to_store_(parser_to_store) {}

void Parser_Dispatch::dispatch() noexcept
{
   Packet *pkt;
   receiver_to_parser_.pop(pkt);
   switch (pkt->protocol)
   {
   case Protocol::FIX:
   {
      FIXMessage *fixMsg{fixparser_.parse(pkt->data.data(), pkt->data.size())};
      parser_to_store_.push(MessageWithVenue<std::variant<FIXMessage *, ITCHMessage, SBEMessage>>(fixMsg, pkt->venue));
      fixparser_.releaseFIX(fixMsg);
      break;
   }
   case Protocol::ITCH:
   {
      ITCHMessage itchMsg{itchparser_.parse(pkt->data.data())};
      parser_to_store_.push(MessageWithVenue<std::variant<FIXMessage *, ITCHMessage, SBEMessage>>(itchMsg, pkt->venue));
      itchparser_.releaseITCH(itchMsg);
      break;
   }
   case Protocol::SBE:
   {
      SBEMessage sbeMsg{sbeparser_.parse(pkt->data.data())};
      parser_to_store_.push(MessageWithVenue<std::variant<FIXMessage *, ITCHMessage, SBEMessage>>(sbeMsg, pkt->venue));
      sbeparser_.releaseSBE(sbeMsg);
      break;
   }
   default:
      __builtin_unreachable();
   }
}