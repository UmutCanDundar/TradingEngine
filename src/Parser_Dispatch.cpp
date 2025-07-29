#include "Parser_Dispatch.h"

Parser_Dispatch::Parser_Dispatch(spscQueue_t &queue) : queue_(queue) {}

MessageWithVenue<std::variant<FIXMessage, ITCHMessage, SBEMessage>> Parser_Dispatch::dispatch() noexcept
{
   Packet pkt;
   queue_.pop(pkt);
   switch (pkt.protocol)
   {
   case Protocol::FIX:
      return MessageWithVenue<std::variant<FIXMessage, ITCHMessage, SBEMessage>>(fixparser_.parse(pkt.data.data()), pkt.venue);
   case Protocol::ITCH:
      return MessageWithVenue<std::variant<FIXMessage, ITCHMessage, SBEMessage>>(itchparser_.parse(pkt.data.data()), pkt.venue);
   case Protocol::SBE:
      return MessageWithVenue<std::variant<FIXMessage, ITCHMessage, SBEMessage>>(sbeparser_.parse(pkt.data.data()), pkt.venue);
   default:
      __builtin_unreachable();
   }
}