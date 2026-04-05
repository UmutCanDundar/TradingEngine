// ==============================================================================================
// MarketBook
//
// PURPOSE:
// - Maintains aggregated market depth (best bid / best ask) per symbol.
// - Updates price-level quantities based on incoming order lifecycle events.
// - Serves as the authoritative in-memory order book representation.
//
// THREAD SAFETY:
// - Single-writer model assumed (market data processing thread).
// - No internal synchronization or locking.
// - Not safe for concurrent writers.
//
// PERFORMANCE & DESIGN NOTES:
// - Hot-path component.
// - Optimized for frequent updates and best price queries.
// - Uses ordered container to allow O(1) best bid/ask access.
// - Tracks only aggregated quantities per price level (no order-level storage).
// - Current design prioritizes correctness and clarity over premature specialization.
//
// DEVELOPER NOTES:
// - Uses a node-based, sorted container for price-level aggregation.
// - Assumes a limited number of active price levels per symbol under normal market conditions.
// - If the number of price-levels exceed cache limits, alternative data structures or algorithms
//   may be considered in the future to improve locality.
// ===============================================================================================

#pragma once

#include "Order.h"
#include "common.h"
#include "HashTables.h"
#include "Protocol-Venue.h"

#include <array>
#include <vector>
#include <cstdint>

#include <absl/container/btree_map.h>

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

    inline void add_order(const Order &order) noexcept
    {
        if (UNLIKELY(order.isOurOrder))
            return;

        auto &symBook = get_symBook(order);
        auto &tree = symBook.bid_ask_trees_[static_cast<uint8_t>(order.side)];

        auto it = tree.find(order.price);
        if (it == tree.end())
            tree.emplace(order.price, order.quantity);
        else
            it->second += order.quantity;
    }
    inline void modify_order(const Order &order, uint32_t newQuantity) noexcept
    {
        if (UNLIKELY(order.isOurOrder))
            return;

        auto &symBook = get_symBook(order);
        auto &tree = symBook.bid_ask_trees_[static_cast<uint8_t>(order.side)];

        int64_t diff = static_cast<int64_t>(newQuantity) - static_cast<int64_t>(order.quantity);

        auto it = tree.find(order.price);
        if (it != tree.end())
        {
            it->second += diff;
            if (it->second <= 0)
                tree.erase(it);
        }
        else
        {
            if (diff > 0)
                tree[order.price] = static_cast<uint32_t>(diff);
        }
    }
    inline void cancel_order(const Order &order) noexcept
    {
        if (UNLIKELY(order.isOurOrder))
            return;

        auto &symBook = get_symBook(order);
        auto &tree = symBook.bid_ask_trees_[static_cast<uint8_t>(order.side)];

        auto it = tree.find(order.price);
        if (LIKELY(it != tree.end()))
        {
            if (it->second <= order.remaining_quantity)
                tree.erase(it);
            else
                it->second -= order.remaining_quantity;
        }
    }
    inline void delete_order(const Order &order) noexcept
    {
        if (UNLIKELY(order.isOurOrder))
            return;

        auto &symBook = get_symBook(order);
        auto &tree = symBook.bid_ask_trees_[static_cast<uint8_t>(order.side)];

        uint32_t remaining_qty = order.quantity - order.filled_quantity;

        auto it = tree.find(order.price);
        if (LIKELY(it != tree.end()))
        {
            if (it->second <= remaining_qty)
                tree.erase(it);
            else
                it->second -= remaining_qty;
        }
    }
    inline void exec_order(const Order &order) noexcept
    {
        if (UNLIKELY(order.isOurOrder))
            return;

        auto &symBook = get_symBook(order);
        auto &tree = symBook.bid_ask_trees_[static_cast<uint8_t>(order.side)];

        auto it = tree.find(order.price);
        if (LIKELY(it != tree.end()))
        {
            if (it->second <= order.last_exec_quantity)
                tree.erase(it);
            else
                it->second -= order.last_exec_quantity;
        }
    }

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

    void flush(const uint8_t venue_index, const uint32_t symbol_index) noexcept;
};
