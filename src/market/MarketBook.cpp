#include "MarketBook.h"

MarketBook::MarketBook(HashTables &hashtables) noexcept : hashtables_(hashtables) 
{}

void MarketBook::flush(const uint8_t venue_index, const uint32_t symbol_index) noexcept
{
   auto &symbolbook = symbolbooks_[venue_index][symbol_index];
   auto &bid_ask_trees = symbolbook.bid_ask_trees_;
   bid_ask_trees[0].clear();
   bid_ask_trees[1].clear();
}