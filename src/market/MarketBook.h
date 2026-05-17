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
#include <atomic>

#include <absl/container/btree_map.h>

class MarketBook
{
private:
    static constexpr size_t MAX_SYMBOL_COUNT = 512; 

    HashTables &hashtables_;

    struct SymbolBook
    {
        

        std::array<absl::btree_map<int64_t, uint32_t>, 2> bid_ask_trees_;
        std::atomic<int64_t> best_bid{0};
        std::atomic<int64_t> best_ask{0};
        std::atomic<uint32_t> best_bid_qty{0};
        std::atomic<uint32_t> best_ask_qty{0};

        SymbolBook() noexcept = default;
        SymbolBook(const SymbolBook&) = delete;
        SymbolBook& operator=(const SymbolBook&) = delete;
        SymbolBook(SymbolBook&&) = delete;
        SymbolBook& operator=(SymbolBook&&) = delete;
    };

    std::array<std::array<SymbolBook, MAX_SYMBOL_COUNT>, VENUE_COUNT> symbolbooks_;

public:
    MarketBook(HashTables &hashtables) noexcept;

    inline void add_order(const Order &order) noexcept
    {
        if (UNLIKELY(order.isOurOrder))
            return;
        
        auto* symBook = get_symBook(order);
        if(!symBook)
            return;
        
        auto &tree = (*symBook).bid_ask_trees_[static_cast<size_t>(order.side)];

        auto it = tree.find(order.price);
        if (it == tree.end())
        {
            tree.emplace(order.price, order.quantity);
            update_best_bid_ask(*symBook, order.side);  
        }
        else
        {
            it->second += order.quantity;
            update_only_best_qty(*symBook, order.side);
        }
    }
    inline void modify_order(const Order &order, uint32_t newQuantity) noexcept
    {
        if (UNLIKELY(order.isOurOrder))
            return;

        auto* symBook = get_symBook(order);
        if(!symBook)
            return;
        
        auto &tree = (*symBook).bid_ask_trees_[static_cast<size_t>(order.side)];


        int64_t diff = static_cast<int64_t>(newQuantity) - static_cast<int64_t>(order.quantity);

        auto it = tree.find(order.price);
        if (it != tree.end())
        {
            it->second += diff;
            if (it->second <= 0)
                tree.erase(it);

            update_best_bid_ask(*symBook, order.side);
        }
        else
        {
            if (diff > 0)
                tree[order.price] = static_cast<uint32_t>(diff);
            
            update_only_best_qty(*symBook, order.side);
        }

        
    }
    inline void cancel_order(const Order &order) noexcept
    {
        if (UNLIKELY(order.isOurOrder))
            return;

        auto* symBook = get_symBook(order);
        if(!symBook)
            return;

        auto &tree = (*symBook).bid_ask_trees_[static_cast<size_t>(order.side)];


        auto it = tree.find(order.price);
        if (LIKELY(it != tree.end()))
        {
            if (it->second <= order.replaced_quantity)
            {
                tree.erase(it);
                update_best_bid_ask(*symBook, order.side);
            }
            else
            {
                it->second -= order.replaced_quantity;
                update_only_best_qty(*symBook, order.side);
            }
        }
    }
    inline void delete_order(const Order &order) noexcept
    {
        if (UNLIKELY(order.isOurOrder))
            return;

        auto* symBook = get_symBook(order);
        if(!symBook)
            return;
        
        auto &tree = (*symBook).bid_ask_trees_[static_cast<size_t>(order.side)];

        auto it = tree.find(order.price);
        if (LIKELY(it != tree.end()))
        {
            if (it->second <= order.remaining_quantity)
            {
                tree.erase(it);
                update_best_bid_ask(*symBook, order.side);
            }
            else
            {
                it->second -= order.remaining_quantity;
                update_only_best_qty(*symBook, order.side);
            }
        }
    }
    inline void exec_order(const Order &order) noexcept
    {
        if (UNLIKELY(order.isOurOrder))
            return;

        auto* symBook = get_symBook(order);
        if(!symBook)
            return;
        
        auto &tree = (*symBook).bid_ask_trees_[static_cast<size_t>(order.side)];


        auto it = tree.find(order.price);
        if (LIKELY(it != tree.end()))
        {
            if (it->second <= order.last_exec_quantity)
            {
                tree.erase(it);
                update_best_bid_ask(*symBook, order.side);
            }
            else
            {
                it->second -= order.last_exec_quantity;
                update_only_best_qty(*symBook, order.side);
            }
        }
    }

    inline SymbolBook* get_symBook(const Order &order) noexcept
    {
        auto venue_index = static_cast<size_t>(order.venue);
        auto sym_index = order.symbol_index;
        return (sym_index != UINT32_MAX) ? &symbolbooks_[venue_index][sym_index] : nullptr;
    }

    inline int64_t best_bid(const SymbolBook &symBook) const noexcept
    {
        return symBook.best_bid.load(std::memory_order_acquire);
    }
    inline int64_t best_ask(const SymbolBook &symBook) const noexcept
    {
        return symBook.best_ask.load(std::memory_order_acquire);
    }
    inline uint32_t best_bid_qty(const SymbolBook &symBook) const noexcept
    {
        return symBook.best_bid_qty.load(std::memory_order_acquire);
    }
    inline uint32_t best_ask_qty(const SymbolBook &symBook) const noexcept
    {
        return symBook.best_ask_qty.load(std::memory_order_acquire);
    }

    void flush(const uint8_t venue_index, const uint32_t symbol_index) noexcept;

private:

    inline void update_best_bid_ask(SymbolBook &symBook, const Side side) const noexcept
    {
        if(side == Side::Buy)
        {
            update_best_bid(symBook);
            update_best_bid_qty(symBook);
        }
        else
        {
            update_best_ask(symBook);
            update_best_ask_qty(symBook);
        }  
    }

    inline void update_only_best_qty(SymbolBook &symBook, const Side side) const noexcept
    {
        side == Side::Buy ? update_best_bid_qty(symBook) : update_best_ask_qty(symBook);
    }

    inline void update_best_bid(SymbolBook &symBook) const noexcept
    {
        int64_t new_best_bid = symBook.bid_ask_trees_[0].empty() ? 0 : symBook.bid_ask_trees_[0].rbegin()->first;
        symBook.best_bid.store(new_best_bid, std::memory_order_release);  
    }

    inline void update_best_ask(SymbolBook &symBook) const noexcept
    {
        int64_t new_best_ask = symBook.bid_ask_trees_[1].empty() ? 0 : symBook.bid_ask_trees_[1].begin()->first;
        symBook.best_ask.store(new_best_ask, std::memory_order_release);
    }

    inline void update_best_bid_qty(SymbolBook &symBook) const noexcept
    {
        uint32_t new_best_bid_qty = symBook.bid_ask_trees_[0].empty() ? 0 : symBook.bid_ask_trees_[0].rbegin()->second;
        symBook.best_bid_qty.store(new_best_bid_qty, std::memory_order_release);
    }
    inline void update_best_ask_qty(SymbolBook &symBook) const noexcept
    {
        uint32_t new_best_ask_qty = symBook.bid_ask_trees_[1].empty() ? 0 : symBook.bid_ask_trees_[1].begin()->second;
        symBook.best_ask_qty.store(new_best_ask_qty, std::memory_order_release);
    }


};
