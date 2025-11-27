#include "Builder_Dispatch.h"

Builder_Dispatch::Builder_Dispatch(spscInPacketPayloadQueue_t &builder_to_sender, spscOrderQueue_t &risk_to_builder, spscFIXOutSessionQueue_t &parser_to_fixbuilder_out, spscFIXInSessionQueue_t &parser_to_fixbuilder_in, SessionManager &sess_mngr, SoupBinTcp &sbt) noexcept
    : builder_to_sender_(builder_to_sender), risk_to_builder_(risk_to_builder), parser_to_fixbuilder_out_(parser_to_fixbuilder_out), parser_to_fixbuilder_in_(parser_to_fixbuilder_in), sess_mngr_(sess_mngr), sbt_(sbt), builder_table_(makeBuilderLookUpTable()) {}

std::array<std::array<Builder_Dispatch::BuilderFunc, VENUE_COUNT>, PROTOCOL_COUNT> Builder_Dispatch::makeBuilderLookUpTable() noexcept
{
   std::array<std::array<Builder_Dispatch::BuilderFunc, VENUE_COUNT>, PROTOCOL_COUNT> Builder_table{};  // 3/8 being used

   Builder_table[static_cast<size_t>(Protocol::FIX)][static_cast<size_t>(Venue::BIST)] = &Builder_Dispatch::buildFIX;

   Builder_table[static_cast<size_t>(Protocol::OUCH)][static_cast<size_t>(Venue::BIST)] = &Builder_Dispatch::buildOUCH_BIST;
   Builder_table[static_cast<size_t>(Protocol::OUCH)][static_cast<size_t>(Venue::NASDAQ)] = &Builder_Dispatch::buildOUCH_NASDAQ;

   /* Builder_table[static_cast<size_t>(Protocol::SBE)][static_cast<size_t>(Venue::BIST)] = &Builder_Dispatch::buildSBE; */

   return Builder_table;
}

void Builder_Dispatch::dispatch() noexcept
{
   Order* order{nullptr};
   risk_to_builder_.pop(order);

   if(!order)
      return;
   
   (this->*builder_table_[static_cast<size_t>(order->protocol)][static_cast<size_t>(order->venue)])(order);
}