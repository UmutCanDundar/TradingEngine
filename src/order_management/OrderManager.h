// ======================================================================================================
// OrderManager
//
// PURPOSE:
// - This component is responsible for managing the lifecycle of orders across
//   multiple venues, including order pooling, pending order tracking, venue state
//   management, and coordination between market data handlers, risk checks, and
//   downstream strategy and persistence pipelines.
//
// - The class acts as a central orchestration point rather than a data-oriented
//   hot-path structure. Large containers (pools, hash maps, vectors) dominate its
//   memory footprint and access patterns.
//
// THREAD SAFETY:
// - Although multiple threads interact with OrderManager via
//   SPSC queues across different data paths (parsers,
//   risk, strategy), the OrderManager itself is strictly single-threaded,
//   all internal state mutation is confined to the store thread.
//
// - Atomic members are used only where cross-thread visibility is required.
//
// - False sharing is consciously tolerated in some structures where write
//   frequency is low and contention is not on the critical path.
//
// - Inter-thread communication is primarily performed via SPSC queues.
//
// PERFORMANCE & DESIGN NOTES:
// - This class intentionally prioritizes clarity and architectural cohesion
//   over aggressive cache-line level micro-optimization.
//
// - Only filler components are split out to improve readability,
//   with file-level separation and controlled access via friend declarations.
//
// - Most performance-critical data lives inside pool elements or container
//   nodes, to:
//     - Avoid frequent heap allocations
//     - Provide stable memory addresses
//     - Improve cache predictability and reuse
//
// - Access patterns typically involve pointer indirection, making the
//   enclosing object layout less impactful on cache locality.
//
// - Order identifiers are hashed and stored in hash-based containers
//   to achieve average O(1) lookup for order access, modification,
//   and lifecycle transitions.
//
// - Hot-path operations avoid dynamic memory allocation, exception handling.
//
// - Hot-path message dispatch avoids virtual function calls:
//   - Message handling is implemented using std::variant and std::visit
//     combined with if constexpr, enabling compile-time dispatch and
//     eliminating vtable indirection on the hot path.
//
// DEVELOPER NOTES (PRE-PROFILING):
// - This class must be created on the heap due to ~14.5MB Stack usage.
//
// - The current layout assumes low-frequency writes to shared metadata such as
//   venue state and symbol metadata.
//
// - If profiling reveals sustained write contention or cache invalidation
//   pressure, the following refactors are expected:
//     - Extraction of hot mutable state into dedicated cache-aligned structs.(size_t private members,...)
//     - Separation of read-mostly and write-heavy fields into independent
//       structures. (SymbolMeta)
//
// - This class intentionally aggregates multiple responsibilities at this stage:
//    - After sufficient production-like testing and profiling,
//      this logic will be decomposed into specialized components, where:
//          - Order lifecycle management will be handled by a dedicated class
//          - Message handling and routing will be isolated
//          - State updates and bookkeeping will be separated
//          - This class will act purely as a high-level manager/coordinator
//
// ======================================================================================================

#pragma once

#include "Protocol-Venue.h"
#include "common.h"
#include "Order.h"
#include "MessageWithVenue.h"
#include "DbTypes.h"

#include "FillerFIX.h"
#include "FillerITCH_BIST.h"
#include "FillerITCH_NASDAQ.h"
#include "FillerOUCH_BIST.h"
#include "FillerOUCH_NASDAQ.h"

#include <cstdint>
#include <array>
#include <atomic>
#include <memory>
#include <vector>
#include <algorithm>
#include <cstring>
#include <tuple>
#include <utility>

#include <absl/container/flat_hash_map.h>
#include <absl/hash/hash.h>
#include <boost/lockfree/spsc_queue.hpp>
#include <folly/concurrency/ConcurrentHashMap.h>

// =====================
// Forward Declaration
// =====================
class MarketBook;
class HashTables;

namespace BIST
{ 
    struct ITCHOrderBookDirectoryMessage;
    struct ITCHTickSizeTableEntryMessage;
}

namespace NASDAQ 
{
    struct ITCHAddOrderMessage;
    struct ITCHAddOrderMPIDMessage;
    struct ITCHStockDirectoryMessage;
}

// =======================
// Compile-time utilities
// =======================
template <typename T>
concept NASDAQ_ITCHAddMessages =
    std::is_same_v<T, NASDAQ::ITCHAddOrderMessage> ||
    std::is_same_v<T, NASDAQ::ITCHAddOrderMPIDMessage>;

template <typename T>
struct PendingMessage
{
    T msgWithVenue;
    Order *order;

    PendingMessage() = default;
    PendingMessage(T mWithVenue, Order *order) noexcept
        : msgWithVenue(std::move(mWithVenue)), order(order) {}
};

// ===========================
// Metadata & small structures
// ===========================
struct VenueFlags 
{
    static constexpr uint8_t HALTED = 0x01;
    static constexpr uint8_t C_BREAKER = 0x02; 
    
    std::atomic<uint8_t> flags{0x00};
};

struct TickSizeEntry
{
    int64_t price_from = 0;   
    int64_t price_to = 0;   
    uint64_t tick_size = 0; 
};

struct SymbolMeta // writes are rare; cache-line separation intentionally omitted
{
    uint32_t instrument_id;
    uint32_t round_lot_size = 0;
    std::array<std::atomic<TickSizeEntry*>, 5> tick_size_table;
    std::array<char, SYMBOL_SIZE> symbol = {};
    std::atomic<bool> halted{false};
    std::atomic<bool> flushed{false};

    SymbolMeta(uint64_t id = 0, uint32_t round_lot_size = 0, std::array<char, SYMBOL_SIZE> sym = {}, bool halt = false) noexcept : instrument_id(id), round_lot_size(round_lot_size) 
    { 
        halted.store(halt, std::memory_order_relaxed);
        std::memcpy(symbol.data(), sym.data(), SYMBOL_SIZE);
    }
    SymbolMeta(const SymbolMeta&) = delete;
    SymbolMeta& operator=(const SymbolMeta&) = delete;
    SymbolMeta(SymbolMeta&& other) noexcept = delete;
    SymbolMeta& operator=(SymbolMeta&& other) noexcept = delete;
};

struct OrderKey
{
    uint64_t order_id;
    uint32_t instrument_id;
    uint8_t venue;
    uint8_t side;

    bool operator==(const OrderKey &other) const noexcept 
    {
        return (order_id == other.order_id) & (instrument_id == other.instrument_id) & (venue == other.venue) & (side == other.side);
    }
    
};

struct OrderKeyHash
{
    size_t operator()(const OrderKey &k) const noexcept
    {
        return absl::Hash<std::tuple<uint64_t, uint32_t, uint8_t, uint8_t>>{}(std::make_tuple(k.order_id, k.instrument_id, k.venue, k.side));
    }
};

struct OrderHistory
{
    static constexpr uint8_t ORDERHISTORY_SIZE = 64;

    std::array<Order*, ORDERHISTORY_SIZE> orders;
    size_t write_index;

    OrderHistory() : write_index(0) {}

    inline void push(Order* order) noexcept 
    {
        orders[write_index++ & (ORDERHISTORY_SIZE-1)] = order;
    }

    inline void pop(Order *order) noexcept
    {
        for (auto &ptr : orders) {
            if (ptr == order) 
            {
                ptr = nullptr;
                return;
            }
        }
    }

    inline Order *get_recent(size_t offset = 0) const noexcept
    {
        size_t index = write_index;
        size_t found = 0;

        for (size_t i = 0; i < orders.size(); ++i)
        {
            index = (index - 1) & (ORDERHISTORY_SIZE - 1); 
            Order *candidate = orders[index];
            if (LIKELY(candidate != nullptr))
            {
                if (found == offset)
                    return candidate;
                ++found;
            }
        }
        return nullptr; 
    }
};

// ==============================
// Global constants & queue types
// ==============================
inline constexpr size_t ORDER_POOL_CAPACITY = 65536; // 12.6MB Stack!!!
inline constexpr size_t ORDER_MAP_CAPACITY = 32768;
inline constexpr size_t OUR_ORDER_MAP_THRESHOLD = 16384;
inline constexpr size_t PENDING_QUEUE_CAPACITY = 65536;

inline constexpr uint8_t STRATEGY_DONE = 0x01;
inline constexpr uint8_t RISK_DONE = 0x02;
inline constexpr uint8_t BUILDER_DONE = 0x03;

using spscStorePendingQueue_t = boost::lockfree::spsc_queue<PendingMessage<MessageWithVenue<MessageTypes_t>>, boost::lockfree::capacity<PENDING_QUEUE_CAPACITY>>;

class OrderManager
{
    friend class FillerFIX;
    friend class FillerITCH_BIST;
    friend class FillerITCH_NASDAQ;
    friend class FillerOUCH_BIST;
    friend class FillerOUCH_NASDAQ;
    FillerFIX filler_fix_{*this};
    FillerITCH_BIST filler_itch_bist_{*this};
    FillerITCH_NASDAQ filler_itch_nq_{*this};
    FillerOUCH_BIST filler_ouch_bist_{*this};
    FillerOUCH_NASDAQ filler_ouch_nq_{*this};
   
private:
    static constexpr size_t MARKETORDER_LAST_INDEX = 32768;
    static constexpr size_t TICKSIZE_POOL_CAPACITY = 65536; 
    static constexpr size_t TICKSIZE_CAPACITY_FOR_BIST = 32768;
    static constexpr size_t PENDING_ORDER_SIZE = 64;

    bool our_map_full = false;
    bool market_map_full = false;
    
    std::array<TickSizeEntry, TICKSIZE_POOL_CAPACITY> tick_size_entry_pool_; 
    size_t tick_size_bist_next_slot = 0;
    size_t tick_size_nasdaq_next_slot = 32768;

    std::array<Order, ORDER_POOL_CAPACITY> order_pool_; 
    size_t market_next_slot = 0;
    size_t our_next_slot = MARKETORDER_LAST_INDEX;

    std::array<absl::flat_hash_map<uint64_t, std::unique_ptr<SymbolMeta>>, VENUE_COUNT> instrument_cache_; 
    std::array<std::vector<OrderHistory>, VENUE_COUNT> our_orders_all_venue_; 
    std::array<VenueFlags, VENUE_COUNT> venue_flags_; 
    std::array<std::array<std::atomic<Order *>, PENDING_ORDER_SIZE>, VENUE_COUNT> pending_orders_; // Pending OUCH orders for matching against early ITCH messages (linear scan)
    size_t pending_next_slot = 0;

    absl::flat_hash_map<OrderKey, Order*, OrderKeyHash> market_orders_;  
    absl::flat_hash_map<OrderKey, Order*, OrderKeyHash> our_orders_;
    absl::flat_hash_map<uint64_t, Order*> our_orders_wtokenkey_;
    folly::ConcurrentHashMap<uint64_t, Order *> awaitingAck_orders_; // OUCH-FIX orders awaiting exchange acknowledgement
    absl::flat_hash_map<uint32_t, OrderKey> nq_ouch_refnum_ordkey_;
    absl::flat_hash_map<uint64_t, uint32_t> nq_ouch_sym_symid_;

    spscStorePendingQueue_t pending_to_strategy_;
    spscMessageQueue_t &parser_to_store_;
    spscOrderQueue_t &store_to_strategy_;
    spscOrderQueue_t &store_to_strategy_free_slot_;
    spscOrderQueue_t &store_to_risk_;
    spscDbQueue_t &store_to_db_;

    MarketBook &marketbook_;
    HashTables &hashtables_;

public:
    OrderManager(spscMessageQueue_t &parser_to_store,
              spscOrderQueue_t &store_to_strategy,
              spscOrderQueue_t &store_to_strategy_free_slot,
              spscOrderQueue_t &store_to_risk,
              spscDbQueue_t &store_to_db,
              MarketBook &marketbook,
              HashTables &hashtables) noexcept;

    bool store() noexcept;
    void add_order(Order &order) noexcept;
    void add_awaitingAck_order(Order &order) noexcept;

    template <typename T>
    inline void store(const MessageWithVenue<T> &msg) noexcept
    {
        this->update_order(msg);
    }

    inline Order *poll_update() noexcept
    {
        Order *order;
        if (store_to_strategy_.pop(order))
        {
            return order;
        }
        return nullptr;
    }
  
    inline auto const &get_venue_flags(Venue venue) const noexcept 
    { 
        return venue_flags_[static_cast<size_t>(venue)]; 
    }
    
    inline SymbolMeta const *get_symbolmeta(Venue venue, uint32_t instrument_id) const noexcept 
    { 
        auto& instrument_map = instrument_cache_[static_cast<size_t>(venue)];
        auto it = instrument_map.find(instrument_id);
        if (LIKELY(it != instrument_map.end()))
            return it->second.get();
        else
            return nullptr;
    }

private:
    Order* add_our_order(Order *order) noexcept;
    Order* add_market_order(uint64_t order_id, uint32_t instrument_id, Venue venue, uint8_t side) noexcept;
    Order* get_order_from_market_map(uint64_t order_id, uint32_t instrument_id, Venue venue, uint8_t side) noexcept;
    Order* get_order_from_our_map(OrderKey orderkey) noexcept;
    Order* get_order_from_our_map_wtokenkey(uint64_t client_order_id) noexcept;
    Order* get_order_from_awaitingAck_orders(uint64_t client_order_id) noexcept;

    void update_order(const MessageWithVenue<FIXMessage *> &fixMsg) noexcept; // ONLY BIST
    inline void update_order(const MessageWithVenue<FIXSessionMessage *> &) noexcept { return;}
    void update_order(const MessageWithVenue<BIST::ITCHMessage> &itchMsg) noexcept;
    void update_order(const MessageWithVenue<NASDAQ::ITCHMessage> &itchMsg) noexcept;
    void update_order(const MessageWithVenue<BIST::OUCHMessage> &ouchMsg) noexcept;
    void update_order(const MessageWithVenue<NASDAQ::OUCHMessage> &ouchMsg) noexcept;
    
    void handle_instrument_definition(const BIST::ITCHOrderBookDirectoryMessage &msg, Venue venue) noexcept;
    void handle_instrument_definition(const NASDAQ::ITCHStockDirectoryMessage &msg, Venue venue) noexcept;
    void handle_tick_size_definition(const BIST::ITCHTickSizeTableEntryMessage &msg, Venue venue) noexcept;
    void handle_tick_size_definition(auto &tick_size_table, int64_t price_from, int64_t price_to, uint64_t tick_size) noexcept;
    
    void handle_flush_status(const uint8_t venue_index, const uint32_t symbol_index) noexcept;

    void update_venue_halt_status(Venue venue, uint8_t halted, uint8_t circuit_breaker) noexcept;
    void update_symbol_halt_status(uint64_t instrument_id, Venue venue, bool halted) noexcept;
    void update_symbol_flush_status(uint64_t instrument_id, Venue venue, bool flushed) noexcept;

    inline void release_order(OrderHistory &orderhistory, Order &order) noexcept
    {
        store_to_strategy_free_slot_.push(&order);
        our_orders_.erase(OrderKey{order.order_id, order.instrument_id, static_cast<uint8_t>(order.venue), static_cast<uint8_t>(order.side)});
        our_orders_wtokenkey_.erase(order.client_order_id);
        orderhistory.pop(&order);
    }

    inline void release_order(Order &order) noexcept
    {
        store_to_strategy_free_slot_.push(&order);
        market_orders_.erase(OrderKey{order.order_id, order.instrument_id, static_cast<uint8_t>(order.venue), static_cast<uint8_t>(order.side)});
    }

    inline bool is_matched_pending_order(const BIST::ITCHAddOrderMessage &msg) noexcept
    {
        for (auto &atomic_order_ptr : pending_orders_[static_cast<size_t>(Venue::BIST)])
        {
            Order *pending_order = atomic_order_ptr.load(std::memory_order_acquire);
            if (!pending_order)
                continue;

            Side side = (msg.side == 'B') ? Side::Buy : Side::Sell;
            if (pending_order->quantity == msg.quantity &&
                pending_order->price == msg.price &&
                pending_order->instrument_id == msg.order_book_id &&
                pending_order->side == side &&
                pending_order->last_update_time < msg.timestamp_ns &&
                (msg.timestamp_ns - pending_order->last_update_time) < 500'000)
            {
                return true;
            }
        }
        return false;
    }

    template <NASDAQ_ITCHAddMessages T>
    bool is_matched_pending_order(const T& msg) noexcept
    {
        for (auto &atomic_order_ptr : pending_orders_[static_cast<size_t>(Venue::NASDAQ)])
        {
            Order *pending_order = atomic_order_ptr.load(std::memory_order_acquire);
            if (!pending_order)
                continue;

            Side side = (msg.side == 'B') ? Side::Buy : Side::Sell;
            if (pending_order->quantity == msg.shares &&
                pending_order->price == msg.price &&
                pending_order->instrument_id == msg.stock_locate &&
                pending_order->side == side &&
                pending_order->last_update_time < msg.timestamp &&
                (msg.timestamp - pending_order->last_update_time) < 500'000)
            {
                return true;
            }
        }
        return false;
    }

    static inline void copy_symbol(std::array<char, SYMBOL_SIZE> &dest, const std::string_view src) noexcept
    {
        std::memset(dest.data(), 0, SYMBOL_SIZE);
        size_t len = std::min(src.size(), static_cast<size_t>(SYMBOL_SIZE));
        std::memcpy(dest.data(), src.data(), len);
    }

    static inline void copy_order_id(std::array<char, ORDER_IDs_SIZE> &dest, const char* src, size_t len) noexcept
    {
        std::memset(dest.data(), 0, ORDER_IDs_SIZE); 
        std::memcpy(dest.data(), src, len);
    }
};

