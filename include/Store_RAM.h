
#pragma once

#include "Protocol-Venue.h"
#include "Parser_Dispatch.h"
#include "common.h"
#include "HashTables.h"
#include "Order.h"
#include "MarketBook.h"

#include <cstdint>
#include <array>
#include <absl/container/flat_hash_map.h>
#include <absl/hash/hash.h>
#include <boost/lockfree/spsc_queue.hpp>
#include <tbb/concurrent_hash_map.h>
#include <vector>
#include <cstring>
#include <immintrin.h>
#include <bit>
#include <tuple>
#include <atomic>

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
    PendingMessage(const T &msgWithVenue, Order *order) noexcept
        : msgWithVenue(msgWithVenue), order(order) {}
};

struct alignas(64) VenueFlags {
    std::atomic<bool> halted{false};
    std::atomic<bool> circuit_breaker{false};
    uint8_t pad[62];
};

struct TickSizeEntry
{
    int64_t price_from = 0;   // scaled fiyat (örnek: fiyat * 1e4)
    int64_t price_to = 0;    // scaled fiyat
    uint64_t tick_size = 0; // scaled tick size
};

struct alignas(64) SymbolMeta
{
    uint32_t instrument_id;
    uint32_t round_lot_size = 0;
    std::array<std::atomic<TickSizeEntry*>, 5> tick_size_table;
    std::array<char, SYMBOL_SIZE> symbol = {};
    std::atomic<bool> halted{false};
    std::atomic<bool> flushed{false};
    uint8_t pad[31];

    SymbolMeta(uint64_t id = 0, uint32_t round_lot_size = 0 ,std::array<char, SYMBOL_SIZE> sym = {}, bool halt = false) noexcept : instrument_id(id), round_lot_size(round_lot_size) {halted.store(halt, std::memory_order_relaxed);}
    SymbolMeta(const SymbolMeta&) = delete;
    SymbolMeta& operator=(const SymbolMeta&) = delete;
    SymbolMeta(SymbolMeta&& other) noexcept {}
    SymbolMeta& operator=(SymbolMeta&& other) noexcept { return *this; }
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
    std::vector<Order*> orders;
    size_t write_index;

    OrderHistory() : orders(64), write_index(0) {}

    inline void push(Order* order) noexcept {
        orders[write_index & (orders.size()-1)] = order;
        write_index++;
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
            index = (index - 1) & (orders.size() - 1); 
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

inline constexpr size_t ORDER_POOL_CAPACITY = 65536; // 13MB @192B each
inline constexpr size_t ORDER_MAP_CAPACITY = 32768;
inline constexpr size_t OUR_ORDER_MAP_THRESHOLD = 16384;
inline constexpr size_t ORDER_QUEUE_CAPACITY = 65536;
inline constexpr size_t PENDING_QUEUE_CAPACITY = 65536;
inline constexpr uint8_t STRATEGY_DONE = 0x01;
inline constexpr uint8_t RISK_DONE = 0x02;

using spscStorePendingQueue_t = boost::lockfree::spsc_queue<PendingMessage<MessageWithVenue<MessageTypes_t>>, boost::lockfree::capacity<PENDING_QUEUE_CAPACITY>>;
using spscOrderQueue_t = boost::lockfree::spsc_queue<Order *, boost::lockfree::capacity<ORDER_QUEUE_CAPACITY>>;

class Store_RAM
{
private:
    static constexpr size_t MARKETORDER_LAST_INDEX = 32768;
    static constexpr size_t TICKSIZE_POOL_CAPACITY = 65536;
    static constexpr size_t TICKSIZE_CAPACITY_FOR_BIST = 32768;
    static constexpr size_t PENDING_ORDER_SIZE = 64;

    static inline bool our_map_full = false;
    static inline bool market_map_full = false;
    static inline uint64_t bist_trade_id = 0ULL;
   
    std::array<TickSizeEntry, TICKSIZE_POOL_CAPACITY> tick_size_entry_pool_;
    size_t tick_size_bist_next_slot = 0;
    size_t tick_size_nasdaq_next_slot = 32768;

    std::array<Order, ORDER_POOL_CAPACITY> order_pool_;
    size_t market_next_slot = 0;
    size_t our_next_slot = MARKETORDER_LAST_INDEX;

    absl::flat_hash_map<OrderKey, Order*, OrderKeyHash> market_order_map_;  
    absl::flat_hash_map<OrderKey, Order*, OrderKeyHash> our_order_map_;
    absl::flat_hash_map<uint64_t, Order*> our_order_map_wtokenkey_;
    tbb::concurrent_hash_map<uint64_t, Order*> pending_order_map_; // OUCH-FIX orders awaiting exchange acknowledgement

    std::array<absl::flat_hash_map<uint64_t, SymbolMeta>,VENUE_COUNT> instrument_cache_;
    std::array<std::vector<OrderHistory>, VENUE_COUNT> our_orders_all_venue_;
    std::array<VenueFlags, VENUE_COUNT> venue_flags_;
    std::array<std::array<std::atomic<Order *>, PENDING_ORDER_SIZE>, VENUE_COUNT> pending_orders_; // Pending OUCH orders for matching against early ITCH messages (linear scan)
    size_t pending_next_slot = 0;

    spscStorePendingQueue_t pending_to_strategy_;
    spscMessageQueue_t &parser_to_store_;
    spscOrderQueue_t &store_to_risk_;
    spscOrderQueue_t &store_to_strategy_;
    spscOrderQueue_t &store_to_strategy_free_slot_;
    spscDbQueue_t &store_to_db_;

    MarketBook &marketbook_;
    HashTables &hashtables_;

public:
    Store_RAM(spscMessageQueue_t &parser_to_store,
              spscOrderQueue_t &store_to_strategy,
              spscOrderQueue_t &store_to_strategy_free_slot,
              spscOrderQueue_t &store_to_risk,
              spscDbQueue_t &store_to_db,
              MarketBook &marketbook,
              HashTables &hashtables) noexcept;

    inline Order *poll_update() noexcept
    {
        Order *order;
        if (store_to_strategy_.pop(order))
        {
            return order;
        }
        return nullptr;
    }

    template <typename T>
    inline void store(const MessageWithVenue<T> &msg) noexcept
    {
        this->update_order(msg);
    }
    void store() noexcept;

    void add_pending_order(Order *order) noexcept;
    inline auto const &get_venue_flags(Venue venue) const noexcept { return venue_flags_[static_cast<size_t>(venue)]; }
    inline SymbolMeta const *get_symbolmeta(Venue venue, uint32_t instrument_id) const noexcept 
    { 
        auto& instrument_map = instrument_cache_[static_cast<size_t>(venue)];
        auto it = instrument_map.find(instrument_id);
        if (LIKELY(it != instrument_map.end()))
            return &it->second;
        else
            return nullptr;
    }

private:
    Order* add_our_order(Order *order) noexcept;
    Order* add_market_order(uint64_t order_id, uint32_t instrument_id, Venue venue, uint8_t side) noexcept;
    Order* get_order_from_market_map(uint64_t order_id, uint32_t instrument_id, Venue venue, uint8_t side) noexcept;
    Order* get_order_from_our_map(uint64_t order_id, uint32_t instrument_id, Venue venue, uint8_t side) noexcept;
    Order* get_order_from_our_map_wtokenkey(uint64_t client_order_id) noexcept;
    Order* get_order_from_pending_order_map(uint64_t client_order_id) noexcept;

    void update_order(const MessageWithVenue<FIXMessage *> &fixMsg) noexcept; // ONLY BIST
    void update_order(const MessageWithVenue<BIST::ITCHMessage> &itchMsg) noexcept;
    void update_order(const MessageWithVenue<NASDAQ::ITCHMessage> &itchMsg) noexcept;
    void update_order(const MessageWithVenue<BIST::OUCHMessage> &ouchMsg) noexcept;
    void update_order(const MessageWithVenue<SBEMessage> &sbeMsg) noexcept; //  NECESSITY OF SBE IS NOT DETERMINED YET

    void handle_instrument_definition(const SBEInstrumentDefinitionMessage &msg, Venue venue) noexcept;
    void handle_instrument_definition(const BIST::ITCHOrderBookDirectoryMessage &msg, Venue venue) noexcept;
    void handle_instrument_definition(const NASDAQ::ITCHStockDirectoryMessage &msg, Venue venue) noexcept;

    void handle_tick_size_definition(const BIST::ITCHTickSizeTableEntryMessage &msg, Venue venue) noexcept;
    void handle_tick_size_definition(auto &tick_size_table, int64_t price_from, int64_t price_to, uint64_t tick_size) noexcept;

    void handle_flush_status(const uint8_t venue_index, const uint32_t symbol_index) noexcept;

    inline void ReleaseOrder(OrderHistory &orderhistory, Order &order)
    {
        store_to_strategy_free_slot_.push(&order);
        our_order_map_.erase(OrderKey{order.order_id, order.instrument_id, static_cast<uint8_t>(order.venue), static_cast<uint8_t>(order.side)});
        our_order_map_wtokenkey_.erase(order.client_order_id);
        orderhistory.pop(&order);
    }

    inline void ReleaseOrder(Order &order)
    {
        store_to_strategy_free_slot_.push(&order);
        market_order_map_.erase(OrderKey{order.order_id, order.instrument_id, static_cast<uint8_t>(order.venue), static_cast<uint8_t>(order.side)});
    }

    static inline void copy_symbol(std::array<char, SYMBOL_SIZE> &dest, const std::string_view src) noexcept
    {
        size_t len = src.size() < SYMBOL_SIZE ? src.size() : SYMBOL_SIZE;
        std::memcpy(dest.data(), src.data(), len);
        if (len < SYMBOL_SIZE) std::memset(dest.data() + len, 0, SYMBOL_SIZE - len);
    }

    static inline void copy_order_id(std::array<char, ORDER_IDs_SIZE> &dest, const char* src, size_t len) noexcept
    {
        std::memcpy(dest.data(), src, len);

        for (size_t i = 0; i < ORDER_IDs_SIZE; ++i)
        {
            if (dest[i] == ' ' || dest[i] == '\0')
            {
                std::memset(dest.data() + i, 0, ORDER_IDs_SIZE - i);
                break;
            }
        }
    }

    bool is_matched_pending_order(const BIST::ITCHAddOrderMessage &msg) noexcept
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

    inline void update_venue_halt_status(Venue venue, bool halted, bool circuit_breaker) noexcept
    {
        auto venue_index = static_cast<size_t>(venue);

        venue_flags_[venue_index].halted.store(halted, std::memory_order_release);
        venue_flags_[venue_index].circuit_breaker.store(circuit_breaker, std::memory_order_release);
    }

    inline void update_symbol_halt_status(uint64_t instrument_id, Venue venue, bool halted) noexcept
    {
        instrument_cache_[static_cast<size_t>(venue)][instrument_id].halted.store(halted, std::memory_order_release);
    }

    inline void update_symbol_flush_status(uint64_t instrument_id, Venue venue, bool flushed) noexcept
    {
        instrument_cache_[static_cast<size_t>(venue)][instrument_id].halted.store(flushed, std::memory_order_release);
    }

    //===========================================================
    //====================== FIX fillers ========================
    //===========================================================
    inline void fill_fix_new(Order &order, const FIXMessage &msg) noexcept
    {
        order.price = msg.price;
        order.quantity = msg.quantity;
        order.remaining_quantity = msg.quantity;
        copy_order_id(order.fix_org_order_id, msg.order_id.data(), msg.order_id.size());  
        order.order_id = absl::Hash<std::string_view>{}(msg.order_id);
        order.timestamp = static_cast<uint64_t>(msg.transact_time);
        order.last_update_time = order.timestamp;
        order.order_type = static_cast<OrderType>(msg.ord_type - '0');
        order.status = Status::New;
        order.syncState = SyncState::NewSeen;
        order.StatusesPreNew.fill(Status::Unknown);
    }

    inline void fill_fix_partial(Order &order, const FIXMessage &msg) noexcept {
        order.status = Status::Partial;
        order.price = msg.last_price;
        order.filled_quantity = msg.filled_qty;        
        order.last_exec_quantity = msg.last_qty;        
        order.remaining_quantity = msg.leaves_qty;
        order.last_update_time = static_cast<uint64_t>(msg.transact_time);
        if (UNLIKELY(order.syncState == SyncState::WaitingNew))
            order.StatusesPreNew[0] = Status::Partial;
    }

    inline void fill_fix_filled(Order &order, const FIXMessage &msg) noexcept {
        order.status = Status::Filled;
        order.price = msg.last_price;
        order.filled_quantity = msg.filled_qty;       
        order.last_exec_quantity = msg.last_qty;        
        order.remaining_quantity = msg.leaves_qty;     // 0
        order.last_update_time = static_cast<uint64_t>(msg.transact_time);
        if (UNLIKELY(order.syncState == SyncState::WaitingNew))
            order.StatusesPreNew[0] = Status::Filled;
    }

    inline void fill_fix_cancel(Order &order, const FIXMessage &msg) noexcept
    {
        order.status = Status::Cancelled;
        order.filled_quantity = msg.filled_qty;      // iptal öncesi toplam dolum
        order.remaining_quantity = order.quantity - order.filled_quantity;
        order.last_update_time  = static_cast<uint64_t>(msg.transact_time);
        if (UNLIKELY(order.syncState == SyncState::WaitingNew))
            order.StatusesPreNew[1] = Status::Cancelled;
        order.cancelled_count++;
    }

    inline void fill_fix_replaced(Order &order, const FIXMessage &msg) noexcept
    {
        order.price = msg.price;
        order.quantity = msg.quantity;
        order.replaced_quantity = msg.leaves_qty - order.remaining_quantity;
        order.remaining_quantity = msg.leaves_qty;
        copy_order_id(order.client_order_token, msg.cl_ord_id.data(), msg.cl_ord_id.size());
        order.order_id = absl::Hash<std::string_view>{}(msg.order_id);
        order.last_update_time = static_cast<uint64_t>(msg.transact_time);
        order.status = Status::Replaced;
        if (UNLIKELY(order.syncState == SyncState::WaitingNew))
            order.StatusesPreNew[1] = Status::Replaced;
    }

   /*  inline void fill_fix_rejected(Order &order, const FIXMessage &msg) noexcept
    {
        store_to_strategy_free_slot_.push(&order);
    } */

    inline void fill_fix_expired(Order &order, const FIXMessage &msg) noexcept
    {
        order.status = Status::Expired;
        order.filled_quantity = msg.filled_qty;
        order.last_exec_quantity = msg.last_qty;
        order.remaining_quantity = order.quantity - order.filled_quantity;
        order.last_update_time = static_cast<uint64_t>(msg.transact_time);
        if (UNLIKELY(order.syncState == SyncState::WaitingNew))
            order.StatusesPreNew[1] = Status::Expired;
    }

    inline void fill_fix_cancel_reject(Order &order, const FIXMessage &msg) noexcept
    {
        order.last_update_time = static_cast<uint64_t>(msg.transact_time);
    }

    void fill_fix_exec_report(Order &order, const FIXMessage &msg) noexcept;

    //===========================================================
    //=================== BIST ITCH fillers ===================
    //===========================================================
    inline void fill_itch_add(Order &order, const BIST::ITCHAddOrderMessage &msg, Venue venue) noexcept
    {
        order.price = static_cast<int64_t>(msg.price);
        order.quantity = static_cast<uint32_t>(msg.quantity);
        order.remaining_quantity = static_cast<uint32_t>(msg.quantity);
        order.instrument_id = msg.order_book_id;
        order.side = (msg.side == 'B') ? Side::Buy : Side::Sell;
        order.order_id = msg.order_id;

        auto* symbolmeta = get_symbolmeta(venue, order.instrument_id);
        if (symbolmeta)
        {
            order.symbol = symbolmeta->symbol;
            order.symbol_index = hashtables_.getIndex(static_cast<size_t>(venue), order.symbol);
        }
        else
        {
           ReleaseOrder(order);
           return;
        }

        order.status = Status::New;
        order.timestamp = msg.timestamp_ns; 
        order.last_update_time = order.timestamp;
        order.message_type = msg.message_type;
        order.protocol = Protocol::ITCH;
        order.order_type = (order.price < 0) ? OrderType::Limit : OrderType::Market;
        order.cancelled_count = 0;
    }

    inline void fill_itch_exec_report(Order &order, const BIST::ITCHOrderExecutedMessage &msg) noexcept
    {
        order.filled_quantity += msg.executed_quantity;
        order.last_exec_quantity = msg.executed_quantity;
        order.remaining_quantity -= msg.executed_quantity;
        order.last_update_time = msg.timestamp_ns;
        if (order.remaining_quantity > 0)
            order.status = Status::Partial;
        else
            order.status = Status::Filled;

        order.message_type = msg.message_type;
        
    }

    inline void fill_itch_exec_report(Order &order, const BIST::ITCHOrderExecutedWithPriceMessage &msg) noexcept
    {
        fill_itch_exec_report(order, *reinterpret_cast<const BIST::ITCHOrderExecutedMessage *>(&msg));
        order.price = static_cast<int64_t>(msg.trade_price);
    }

    inline void fill_itch_delete(Order &order, const BIST::ITCHOrderDeleteMessage &msg) noexcept
    {
        order.status = Status::Cancelled;
        order.remaining_quantity = order.quantity;
        order.last_update_time = msg.timestamp_ns;
        order.cancelled_count++;
        order.message_type = msg.message_type;
        
    }

    inline void fill_itch_trade(Order &order, const BIST::ITCHTradeMessage &msg, Venue venue) noexcept
    {
        if (UNLIKELY(order.isOurOrder))
            return;

        order.order_id = ++bist_trade_id; // BIST trade mesajlarında order_id yok, trade_id olarak artan bir değer kullanıyoruz
        order.filled_quantity += msg.quantity;
        order.last_exec_quantity = msg.quantity;
        order.last_update_time = msg.timestamp_ns;
        order.remaining_quantity = 0;
        order.instrument_id = msg.order_book_id;

        auto *symbolmeta = get_symbolmeta(venue, order.instrument_id);
        if (symbolmeta)
        {
            order.symbol = symbolmeta->symbol;
            order.symbol_index = hashtables_.getIndex(static_cast<size_t>(venue), order.symbol);
        }
        else
        {
            ReleaseOrder(order);
            return;
        }

        order.price = static_cast<int64_t>(msg.trade_price);
        order.status = Status::Filled;
        if(msg.side == ' ')
            order.side = Side::Unknown;
        else 
            order.side = (msg.side == 'B') ? Side::Buy : Side::Sell;

        order.message_type = msg.message_type;
    }


    //===========================================================
    //=================== NASDAQ ITCH fillers ===================
    //=========================================================== 
    inline void fill_itch_add(Order &order, const NASDAQ::ITCHAddOrderMessage &msg, Venue venue) noexcept
    {
        order.price = static_cast<int64_t>(msg.price);
        order.quantity = msg.shares;
        order.remaining_quantity = msg.shares;
        order.instrument_id = msg.stock_locate;

        auto *symbolmeta = get_symbolmeta(venue, order.instrument_id);
        if (symbolmeta)
        {
            order.symbol = symbolmeta->symbol;
            order.symbol_index = hashtables_.getIndex(static_cast<size_t>(venue), order.symbol);
        }
        else
        {
            ReleaseOrder(order);
            return;
        }

        order.side = (msg.side == 'B') ? Side::Buy : Side::Sell;
        order.status = Status::New;

        order.order_id = msg.order_ref;
        order.timestamp = msg.timestamp; 
        order.last_update_time = order.timestamp;

        order.message_type = msg.message_type;
        order.protocol = Protocol::ITCH;
        order.order_type = (order.price < 0) ? OrderType::Limit : OrderType::Market;
       
    }

    inline void fill_itch_add(Order &order, const NASDAQ::ITCHAddOrderMPIDMessage &msg, Venue venue) noexcept
    {
        order.price = static_cast<int64_t>(msg.price);
        order.quantity = msg.shares;
        order.instrument_id = msg.stock_locate;
        order.remaining_quantity = msg.shares;
        copy_symbol(order.symbol, msg.stock);
        
        auto *symbolmeta = get_symbolmeta(venue, order.instrument_id);
        if (symbolmeta)
        {
            order.symbol = symbolmeta->symbol;
            order.symbol_index = hashtables_.getIndex(static_cast<size_t>(venue), order.symbol);
        }
        else
        {
            ReleaseOrder(order);
            return;
        }

        order.side = (msg.side == 'B') ? Side::Buy : Side::Sell;
        order.status = Status::New;
        order.order_id = msg.order_ref;
        order.timestamp = msg.timestamp;
        order.last_update_time = order.timestamp;
        order.message_type = msg.message_type;
        order.protocol = Protocol::ITCH;
        order.order_type = (order.price < 0) ? OrderType::Limit : OrderType::Market;
        
    }

    inline void fill_itch_cancel(Order &order, const NASDAQ::ITCHCancelMessage &msg) noexcept
    {
        order.status = Status::Cancelled;
        order.last_update_time = msg.timestamp;
        order.remaining_quantity = msg.cancelled_shares;
        order.cancelled_count++;
        order.message_type = msg.message_type;
    }

    inline void fill_itch_exec_report(Order &order, const NASDAQ::ITCHExecutedMessage &msg) noexcept
    {
        order.last_update_time = msg.timestamp;
        order.filled_quantity += msg.executed_shares;
        order.last_exec_quantity = msg.executed_shares;
        order.remaining_quantity -= msg.executed_shares;
        if (order.remaining_quantity > 0)
            order.status = Status::Partial;
        else
            order.status = Status::Filled;

        order.message_type = msg.message_type;
    }

    inline void fill_itch_exec_report(Order &order, const NASDAQ::ITCHExecutedWithPriceMessage &msg) noexcept
    {
        fill_itch_exec_report(order, *reinterpret_cast<const NASDAQ::ITCHExecutedMessage *>(&msg));
        order.price = static_cast<int64_t>(msg.execution_price);
    }

    inline void fill_itch_delete(Order &order, const NASDAQ::ITCHDeleteMessage &msg) noexcept
    {
        order.status = Status::Cancelled;
        order.remaining_quantity = order.quantity - order.filled_quantity;
        order.last_update_time = msg.timestamp;
        order.message_type = msg.message_type;
    }

    inline void fill_itch_replace(Order &order, const NASDAQ::ITCHReplaceMessage &msg, Venue venue) noexcept 
    {
        order.order_id = msg.new_order_ref;
        order.price = msg.price;
        order.quantity = msg.shares;
        order.instrument_id = msg.stock_locate;
        order.replaced_quantity = order.remaining_quantity;
        order.filled_quantity = 0;
        order.remaining_quantity = msg.shares;

        auto *symbolmeta = get_symbolmeta(venue, order.instrument_id);
        if (symbolmeta)
        {
            order.symbol = symbolmeta->symbol;
            order.symbol_index = hashtables_.getIndex(static_cast<size_t>(venue), order.symbol);
        }
        else
        {
            ReleaseOrder(order);
            return;
        }

        order.message_type = msg.message_type;
        order.status = Status::Replaced;
        order.protocol = Protocol::ITCH;
        order.last_update_time = msg.timestamp;
        order.order_type = (order.price < 0) ? OrderType::Limit : OrderType::Market;
        
    }

    inline void fill_itch_trade(Order &order, const NASDAQ::ITCHTradeMessage &msg, Venue venue) noexcept
    {
        if (UNLIKELY(order.isOurOrder))
            return;

        order.order_id = msg.order_ref; 
        order.filled_quantity += msg.shares;
        order.last_exec_quantity = msg.shares;
        order.last_update_time = msg.timestamp;
        order.remaining_quantity = 0;
        order.instrument_id = msg.stock_locate;

        auto *symbolmeta = get_symbolmeta(venue, order.instrument_id);
        if (symbolmeta)
        {
            order.symbol = symbolmeta->symbol;
            order.symbol_index = hashtables_.getIndex(static_cast<size_t>(venue), order.symbol);
        }
        else
        {
            ReleaseOrder(order);
            return;
        }

        order.price = static_cast<int64_t>(msg.price);
        order.status = Status::Filled;
        order.message_type = msg.message_type;
    }

    //===========================================================
    //=================== BIST OUCH fillers ===================
    //===========================================================
    inline void fill_ouch_accepted(Order &order, const BIST::OUT::OUCHOrderAcceptedMessage &msg) noexcept
    {
        order.price = static_cast<int64_t>(msg.price);
        order.quantity = static_cast<uint32_t>(msg.quantity);
        order.remaining_quantity = static_cast<uint32_t>(msg.quantity);
        order.filled_quantity = order.quantity - order.remaining_quantity;
        order.status = Status::New;
        order.order_id = msg.order_id;
        order.timestamp = msg.timestamp;
        order.last_update_time = order.timestamp;
    }

    inline void fill_ouch_replaced(Order &order, const BIST::OUT::OUCHOrderReplacedMessage &msg) noexcept
    {
        if(msg.order_state == 2)
            return;
        
        order.price = static_cast<int64_t>(msg.price);
        order.replaced_quantity = static_cast<int32_t>(msg.quantity - order.remaining_quantity);  // delta qty
        order.remaining_quantity = static_cast<uint32_t>(msg.quantity);  // open qty
        order.quantity += order.replaced_quantity;                        // total qty
        order.status = Status::Replaced;
        order.order_id = msg.order_id;
        order.last_update_time = order.timestamp;
    }

    inline void fill_ouch_cancelled(Order &order, const BIST::OUT::OUCHOrderCancelledMessage &msg) noexcept
    {
        order.last_update_time = order.timestamp;
        order.status = Status::Cancelled;
        order.cancelled_count++;
    }
    inline void fill_ouch_executed(Order &order, const BIST::OUT::OUCHOrderExecutedMessage &msg) noexcept
    {
        order.price = static_cast<int64_t>(msg.trade_price);
        order.filled_quantity += msg.traded_quantity;
        order.last_exec_quantity = msg.traded_quantity;
        order.last_update_time = msg.timestamp;
        if (order.filled_quantity < order.quantity)
            order.status = Status::Partial;
        else
            order.status = Status::Filled;
    }

    inline void fill_ouch_rejected(Order &order, const BIST::OUT::OUCHOrderRejectedMessage &msg) noexcept
    {
        if(order.status == Status::Unknown) 
        {
            store_to_strategy_free_slot_.push(&order);
        }
        else
        {
            order.last_update_time = msg.timestamp;
            order.canModify = 0x00;
        }
    }

    //===========================================================
    //======================= SBE fillers =======================  NECESSITY OF SBE IS NOT DETERMINED YET
    //===========================================================
    inline void fill_sbe_add(Order &order, const SBEAddOrderMessage &msg, Venue venue) noexcept
    {
        auto &instrument_map = instrument_cache_[std::underlying_type_t<Venue>(venue)];

        order.price = static_cast<int64_t>(msg.price);
        order.quantity = msg.quantity;
        order.side = (msg.side == 0) ? Side::Buy : Side::Sell;
        order.status = Status::New;
        order.instrument_id = msg.header.schemaId; // veya header.version kullanılabilir

        auto it = instrument_map.find(order.instrument_id);
        if (LIKELY(it != instrument_map.end()))
        {
            order.symbol = it->second.symbol;
            order.symbol_index = hashtables_.getIndex(static_cast<uint8_t>(venue),order.symbol);
        }

        order.timestamp = msg.header.version;
        order.last_update_time = order.timestamp;

        order.message_type = static_cast<uint8_t>(msg.header.templateId);
        order.protocol = Protocol::SBE;
        order.order_type = (order.price < 0) ? OrderType::Limit : OrderType::Market;
        order.cancelled_count = 0;
        // order.timestamp_ns = msg.header.version;
    }

    inline void fill_sbe_modify(Order &order, const SBEModifyOrderMessage &msg) noexcept
    {
        order.quantity = msg.newQuantity;
        order.last_update_time = msg.header.version;
    }

    inline void fill_sbe_delete(Order &order, const SBEDeleteOrderMessage &msg) noexcept
    {
        order.status = Status::Cancelled;
        order.remaining_quantity = order.quantity - order.filled_quantity;
        order.last_update_time = msg.header.version;
        order.cancelled_count++;
    }

    inline void fill_sbe_trade(Order &order, const SBETradeMessage &msg, Venue venue) noexcept
    {
        if (UNLIKELY(order.isOurOrder))
            return;
    
        auto &instrument_map = instrument_cache_[std::underlying_type_t<Venue>(venue)];

        order.price = static_cast<int64_t>(msg.price);
        order.filled_quantity += msg.quantity;
        order.last_exec_quantity = msg.quantity;
        order.instrument_id = msg.header.schemaId;

        if (order.filled_quantity < order.quantity)
            order.status = Status::Partial;
        else
            order.status = Status::Filled;

        auto it = instrument_map.find(order.instrument_id);
        if (LIKELY(it != instrument_map.end()))
        {
            order.symbol = it->second.symbol;
            order.symbol_index = hashtables_.getIndex(static_cast<uint8_t>(venue),order.symbol);
        }

        order.last_update_time = msg.header.version;
        order.message_type = static_cast<uint8_t>(msg.header.templateId);
        order.protocol = Protocol::SBE;
        order.cancelled_count = 0;
    }
};

