#include "Parser_Dispatch.h"

Parser_Dispatch::Parser_Dispatch(spscPacketQueue_t &receiver_to_parser, spscMessageQueue_t &parser_to_store, spscFIXOutSessionQueue_t &parser_to_fixbuilder_out, spscFIXInSessionQueue_t& parser_to_fixbuilder_in, Session_FIX & session) noexcept
    : receiver_to_parser_(receiver_to_parser), parser_to_store_(parser_to_store), parser_to_fixbuilder_out_(parser_to_fixbuilder_out), session_(session), parser_to_fixbuilder_in_(parser_to_fixbuilder_in), parser_table_(makeParserLookUpTable()) {}

std::array<std::array<Parser_Dispatch::ParserFunc, VENUE_COUNT>, PROTOCOL_COUNT> Parser_Dispatch::makeParserLookUpTable() noexcept
{
   std::array<std::array<Parser_Dispatch::ParserFunc, VENUE_COUNT>, PROTOCOL_COUNT> parser_table{};

   parser_table[static_cast<size_t>(Protocol::FIX)][static_cast<size_t>(Venue::BIST)] = &Parser_Dispatch::parseFIX;

   parser_table[static_cast<size_t>(Protocol::ITCH)][static_cast<size_t>(Venue::BIST)] = &Parser_Dispatch::parseITCH_BIST;
   parser_table[static_cast<size_t>(Protocol::ITCH)][static_cast<size_t>(Venue::NASDAQ)] = &Parser_Dispatch::parseITCH_NASDAQ;

   parser_table[static_cast<size_t>(Protocol::SBE)][static_cast<size_t>(Venue::BIST)] = &Parser_Dispatch::parseSBE;

   return parser_table;
}

void Parser_Dispatch::dispatch() noexcept
{
   Packet *pkt;
   receiver_to_parser_.pop(pkt);
   (this->*parser_table_[static_cast<size_t>(pkt->protocol)][static_cast<size_t>(pkt->venue)])(pkt);
}