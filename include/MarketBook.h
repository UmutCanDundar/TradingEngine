// TopOfBook.h
#pragma once

#include "Order.h"
#include "common.h"
#include "HashTables.h"
#include "Protocol-Venue.h"

#include <array>
#include <vector>
#include <cstdint>
#include <absl/container/btree_map.h> // or std::map if you prefer

class MarketBook
{
private:
    HashTables &hashtables_;

    struct SymbolBook
    {
        std::array<absl::btree_map<int64_t, uint32_t>, 2> bid_ask_trees_;
    };

    std::array<std::vector<SymbolBook>, VENUE_COUNT> symbolbooks_;

public:
    MarketBook(HashTables &hashtables) noexcept;

    void add_order(const Order &order) noexcept;
    void modify_order(const Order &order, uint32_t newQuantity) noexcept;
    void cancel_order(const Order &order) noexcept;
    void delete_order(const Order &order) noexcept;
    void exec_order(const Order &order) noexcept;

    inline SymbolBook &get_symBook(const Order &order) noexcept
    {
        auto venue_index = static_cast<std::underlying_type_t<Venue>>(order.venue);
        return symbolbooks_[venue_index][hashtables_.getIndex(venue_index, order.symbol)];
    }

    inline int64_t best_bid(const SymbolBook &symBook) const noexcept
    {
        return symBook.bid_ask_trees_[0].empty() ? 0LL : symBook.bid_ask_trees_[0].rbegin()->first;
    }

    inline int64_t best_ask(const SymbolBook &symBook) const noexcept
    {
        return symBook.bid_ask_trees_[1].empty() ? 0LL : symBook.bid_ask_trees_[1].begin()->first;
    }

    inline void flush(const uint8_t venue_index, const uint32_t symbol_index) noexcept
    {
        auto &symbolbook = symbolbooks_[venue_index][symbol_index];
        auto &bid_ask_trees = symbolbook.bid_ask_trees_;
        bid_ask_trees[0].clear();
        bid_ask_trees[1].clear();
    }
};
