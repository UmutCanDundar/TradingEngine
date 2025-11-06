#include "Parser_Dispatch.h"

Parser_Dispatch::Parser_Dispatch(spscPacketQueue_t &receiver_to_parser, spscMessageQueue_t &parser_to_store, spscFIXSessionQueue_t &parser_to_fixbuilder, Session_FIX &session) noexcept
    : receiver_to_parser_(receiver_to_parser), parser_to_store_(parser_to_store), parser_to_fixbuilder_(parser_to_fixbuilder), session_(session) {}

std::array<std::array<Parser_Dispatch::ParserFunc, VENUE_COUNT>, PROTOCOL_COUNT> Parser_Dispatch::makeParserLookUpTable() noexcept
{
   std::array<std::array<Parser_Dispatch::ParserFunc, VENUE_COUNT>, PROTOCOL_COUNT> parser_table{};

   parser_table[static_cast<size_t>(Protocol::FIX)][static_cast<size_t>(Venue::BIST)] = &Parser_Dispatch::parseFIX;

   parser_table[static_cast<size_t>(Protocol::ITCH)][static_cast<size_t>(Venue::BIST)] = &Parser_Dispatch::parseITCH_BIST;
   parser_table[static_cast<size_t>(Protocol::ITCH)][static_cast<size_t>(Venue::NASDAQ)] = &Parser_Dispatch::parseITCH_NASDAQ;

   parser_table[static_cast<size_t>(Protocol::SBE)][static_cast<size_t>(Venue::BIST)] = &Parser_Dispatch::parseSBE;

   return parser_table;
}

std::array<std::array<Parser_Dispatch::ParserFunc, VENUE_COUNT>, PROTOCOL_COUNT> Parser_Dispatch::parser_table_ = makeParserLookUpTable();

void Parser_Dispatch::dispatch() noexcept
{
   Packet *pkt;
   receiver_to_parser_.pop(pkt);
   parser_table_[static_cast<size_t>(pkt->protocol)][static_cast<size_t>(pkt->venue)](this, pkt);
}