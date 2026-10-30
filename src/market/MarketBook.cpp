// ==============================  BTREE_MAP-VECTOR =========================================
#include "MarketBook.h"

MarketBook::MarketBook() noexcept  {}

void MarketBook::flush(uint8_t venue_index, uint32_t symbol_index) noexcept
{
   auto &symbolbook = symbolbooks_[venue_index][symbol_index];
   auto &bid_ask_trees = symbolbook.bid_ask_trees_;
   bid_ask_trees[0].clear();
   bid_ask_trees[1].clear();
}