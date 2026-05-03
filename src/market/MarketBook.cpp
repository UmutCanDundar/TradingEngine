#include "MarketBook.h"

MarketBook::MarketBook(HashTables &hashtables) noexcept : hashtables_(hashtables) 
{
   for (size_t venue = 0; venue < VENUE_COUNT; venue++) {
      size_t symbol_count = hashtables_.get_symbol_count(venue);
      symbolbooks_[venue].resize(symbol_count);
   }
}

void MarketBook::flush(const uint8_t venue_index, const uint32_t symbol_index) noexcept
{
   auto &symbolbook = symbolbooks_[venue_index][symbol_index];
   auto &bid_ask_trees = symbolbook.bid_ask_trees_;
   bid_ask_trees[0].clear();
   bid_ask_trees[1].clear();
}