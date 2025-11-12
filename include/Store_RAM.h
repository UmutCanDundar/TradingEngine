
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
#include <vector>
#include <cstring>
#include <immintrin.h>
#include <bit>
#include <tuple>
#include <atomic>

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
    std::array<std::atomic<TickSizeEntry*>, 5> tick_size_table;
    std::array<char, SYMBOL_SIZE> symbol = {};
    std::atomic<bool> halted{false};
    std::atomic<bool> flushed{false};
    uint8_t pad[31];

    SymbolMeta(uint64_t id = 0, std::array<char, SYMBOL_SIZE> sym = {}, bool halt = false) noexcept {instrument_id = id; symbol = sym; halted.store(halt, std::memory_order_relaxed);}
    SymbolMeta(const SymbolMeta&) = delete;
    SymbolMeta& operator=(const SymbolMeta&) = delete;
    SymbolMeta(SymbolMeta&& other) noexcept {}
    SymbolMeta& operator=(SymbolMeta&& other) noexcept { return *this; }
};

struct OrderKey
{
    uint64_t order_id;
    uint32_t instrument_id;
    Venue venue;
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
        return absl::Hash<std::tuple<uint64_t, uint32_t, Venue, uint8_t>>{}(std::make_tuple(k.order_id, k.instrument_id, k.venue, k.side));
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
        size_t idx = write_index;
        size_t found = 0;

        for (size_t i = 0; i < orders.size(); ++i)
        {
            idx = (idx - 1) & (orders.size() - 1); 
            Order *candidate = orders[idx];
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

inline constexpr size_t ORDER_QUEUE_CAPACITY = 65536;
inline constexpr size_t PENDING_QUEUE_CAPACITY = 65536;

inline constexpr size_t ORDER_POOL_CAPACITY = 65536; // 8MB @128B each
inline constexpr size_t ORDER_MAP_CAPACITY = 32768;
inline constexpr size_t OUR_ORDER_MAP_THRESHOLD = 16384;

inline constexpr uint8_t STRATEGY_DONE = 0x01;
inline constexpr uint8_t RISK_DONE = 0x02;

using spscStorePendingQueue_t = boost::lockfree::spsc_queue<PendingMessage<MessageWithVenue<MessageTypes_t>>, boost::lockfree::capacity<PENDING_QUEUE_CAPACITY>>;
using spscOrderQueue_t = boost::lockfree::spsc_queue<Order *, boost::lockfree::capacity<ORDER_QUEUE_CAPACITY>>;
using spscDbQueue_t = boost::lockfree::spsc_queue<std::variant<Order *, MessageWithVenue<FIXMessage *>, MessageWithVenue<BIST::ITCHMessage>, MessageWithVenue<NASDAQ::ITCHMessage>, MessageWithVenue<SBEMessage>>, boost::lockfree::capacity<ORDER_QUEUE_CAPACITY>>;

class Store_RAM
{
private:
    static constexpr size_t TICKSIZE_POOL_CAPACITY = 65536;
    static constexpr size_t TICKSIZE_CAPACITY_FOR_BIST = 32768; 
    
    static constexpr size_t MARKETORDER_LAST_INDEX = 32768;
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
    
    std::array<absl::flat_hash_map<uint64_t, SymbolMeta>,VENUE_COUNT> instrument_cache_;

    std::array<std::vector<OrderHistory>, VENUE_COUNT> our_orders_all_venue_;
    std::array<VenueFlags, VENUE_COUNT> venue_flags_;

    spscMessageQueue_t &parser_to_store_;
    spscStorePendingQueue_t pending_to_strategy_;
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

    Order *add_our_order(Order *order) noexcept;
    inline auto const &get_venue_flags(Venue venue) const noexcept { return venue_flags_[static_cast<std::underlying_type_t<Venue>>(venue)]; }
    inline auto const &get_symbolmeta(Venue venue, uint32_t instrument_id) const noexcept { return instrument_cache_[static_cast<std::underlying_type_t<Venue>>(venue)].find(instrument_id)->second; }

private:
    Order *add_market_order(uint64_t order_id, uint32_t instrument_id, Venue venue, uint8_t side) noexcept;
    Order *get_order(uint64_t order_id, uint32_t instrument_id, Venue venue, uint8_t side) noexcept;

    void update_order(const MessageWithVenue<FIXMessage *> &fixMsg) noexcept;
    void update_order(const MessageWithVenue<BIST::ITCHMessage> &itchMsg) noexcept;
    void update_order(const MessageWithVenue<NASDAQ::ITCHMessage> &itchMsg) noexcept;
    void update_order(const MessageWithVenue<SBEMessage> &sbeMsg) noexcept;

    void handle_instrument_definition(const SBEInstrumentDefinitionMessage *msg, Venue venue) noexcept;
    void handle_instrument_definition(const BIST::ITCHOrderBookDirectoryMessage *msg, Venue venue) noexcept;
    void handle_instrument_definition(const NASDAQ::ITCHStockDirectoryMessage *msg, Venue venue) noexcept;

    void handle_tick_size_definition(const BIST::ITCHTickSizeTableEntryMessage *msg, Venue venue) noexcept;
    void handle_tick_size_definition(auto &tick_size_table, int64_t price_from, int64_t price_to, uint64_t tick_size) noexcept;

    void handle_flush_status(const uint8_t venue_index, const uint32_t symbol_index) noexcept;

    inline void ReleaseOrder(OrderKey &orderkey, OrderHistory &orderhistory, Order *order)
    {
        store_to_strategy_free_slot_.push(order);
        our_order_map_.erase(orderkey);
        orderhistory.pop(order);
    }

    static inline void copy_symbol(std::array<char, SYMBOL_SIZE> &dest, const std::string_view src) noexcept
    {
        size_t len = src.size() < SYMBOL_SIZE ? src.size() : SYMBOL_SIZE;
        std::memcpy(dest.data(), src.data(), len);
        if (len < SYMBOL_SIZE) std::memset(dest.data() + len, 0, SYMBOL_SIZE - len);
    }

    static inline void copy_symbol(std::array<char, SYMBOL_SIZE> &dest, const char* src) noexcept
    {
        std::memcpy(dest.data(), src, SYMBOL_SIZE);

        for (size_t i = 0; i < SYMBOL_SIZE; ++i)
        {
            if (dest[i] == ' ' || dest[i] == '\0')
            {
                std::memset(dest.data() + i, 0, SYMBOL_SIZE - i);
                break;
            }
        }
    }

    inline void update_venue_halt_status(Venue venue, bool halted, bool circuit_breaker) noexcept
    {
        auto venue_index = static_cast<std::underlying_type_t<Venue>>(venue);

        venue_flags_[venue_index].halted.store(halted, std::memory_order_release);
        venue_flags_[venue_index].circuit_breaker.store(circuit_breaker, std::memory_order_release);
    }

    inline void update_symbol_halt_status(uint64_t instrument_id, Venue venue, bool halted) noexcept
    {
        instrument_cache_[static_cast<std::underlying_type_t<Venue>>(venue)][instrument_id].halted.store(halted, std::memory_order_release);
    }

    inline void update_symbol_flush_status(uint64_t instrument_id, Venue venue, bool flushed) noexcept
    {
        instrument_cache_[static_cast<std::underlying_type_t<Venue>>(venue)][instrument_id].halted.store(flushed, std::memory_order_release);
    }

    //===========================================================
    //====================== FIX fillers ========================
    //===========================================================
    inline void fill_fix_new(Order &order, const FIXMessage *msg, Venue venue) noexcept
    {
        order.price = msg->price;
        order.quantity = msg->quantity;
        copy_symbol(order.symbol, msg->symbol);
        order.symbol_index = hashtables_.getIndex(static_cast<uint8_t>(venue),order.symbol);
        order.side = (msg->side == '1') ? Side::Buy : Side::Sell; // FIX: '1'=Buy, '2'=Sell
        order.venue = venue;

        order.order_id = absl::Hash<std::string_view>{}(msg->order_id);
        order.client_order_id = absl::Hash<std::string_view>{}(msg->cl_ord_id);

        order.timestamp = static_cast<uint64_t>(msg->transact_time);
        order.last_update_time = order.timestamp;

        order.message_type = msg->msg_type;
        order.protocol = Protocol::FIX;
        order.instrument_id = msg->instrument_id;

        order.time_in_force = static_cast<TimeInForce>(msg->time_in_force);
        order.order_type = static_cast<OrderType>(msg->ord_type - '0');

        order.status = Status::New;
        order.syncState = SyncState::NewSeen;
        order.StatusesPreNew.fill(Status::Unknown);
        order.cancelled_count = 0; 
        
    }

    inline void fill_fix_partial(Order &order, const FIXMessage *msg) noexcept {
        order.status = Status::Partial;
        order.price = msg->last_price;
        order.filled_quantity = msg->filled_qty;        
        order.last_exec_quantity = msg->last_qty;        
        order.remaining_quantity = msg->leaves_qty;
        order.last_update_time = static_cast<uint64_t>(msg->transact_time);
        if (UNLIKELY(order.syncState == SyncState::WaitingNew))
            order.StatusesPreNew[0] = Status::Partial;
    }

    inline void fill_fix_filled(Order &order, const FIXMessage *msg) noexcept {
        order.status = Status::Filled;
        order.price = msg->last_price;
        order.filled_quantity = msg->filled_qty;       
        order.last_exec_quantity = msg->last_qty;        
        order.remaining_quantity = msg->leaves_qty;     // 0
        order.last_update_time = static_cast<uint64_t>(msg->transact_time);
        if (UNLIKELY(order.syncState == SyncState::WaitingNew))
            order.StatusesPreNew[0] = Status::Filled;
    }

    inline void fill_fix_cancel(Order &order, const FIXMessage *msg) noexcept
    {
        order.status = Status::Cancelled;
        order.filled_quantity = msg->filled_qty;      // iptal öncesi toplam dolum
        order.remaining_quantity = order.quantity - order.filled_quantity;
        order.last_update_time  = static_cast<uint64_t>(msg->transact_time);
        if (UNLIKELY(order.syncState == SyncState::WaitingNew))
            order.StatusesPreNew[1] = Status::Cancelled;
        order.cancelled_count++;
    }

    inline void fill_fix_rejected(Order &order, const FIXMessage *msg) noexcept
    {
        order.status = Status::Rejected;
        order.last_update_time = static_cast<uint64_t>(msg->transact_time);
        if (UNLIKELY(order.syncState == SyncState::WaitingNew))
            order.StatusesPreNew[1] = Status::Rejected;
    }

    inline void fill_fix_expired(Order &order, const FIXMessage *msg) noexcept
    {
        order.status = Status::Expired;
        order.filled_quantity = msg->filled_qty;
        order.last_exec_quantity = msg->last_qty;
        order.remaining_quantity = order.quantity - order.filled_quantity;
        order.last_update_time = static_cast<uint64_t>(msg->transact_time);
        if (UNLIKELY(order.syncState == SyncState::WaitingNew))
            order.StatusesPreNew[1] = Status::Expired;
    }

    inline void fill_fix_cancel_reject(Order &order, const FIXMessage *msg) noexcept
    {
        order.last_update_time = static_cast<uint64_t>(msg->transact_time);
    }

    void fill_fix_exec_report(Order &order, const FIXMessage *msg, Venue venue) noexcept;

    //===========================================================
    //=================== BIST ITCH fillers ===================
    //===========================================================
    inline void fill_itch_add(Order &order, const BIST::ITCHAddOrderMessage *msg, Venue venue) noexcept
    {
        auto &instrument_map = instrument_cache_[std::underlying_type_t<Venue>(venue)];

        order.price = static_cast<int64_t>(msg->price);
        order.quantity = static_cast<uint32_t>(msg->quantity);
        order.filled_quantity = 0;
        order.instrument_id = msg->order_book_id;

        auto it = instrument_map.find(order.instrument_id);
        if (LIKELY(it != instrument_map.end()))
        {
            order.symbol = it->second.symbol;
            order.symbol_index = hashtables_.getIndex(static_cast<uint8_t>(venue), order.symbol);
        }

        order.side = (msg->side == 'B') ? Side::Buy : Side::Sell;
        order.status = Status::New;

        order.order_id = msg->order_id;
        order.timestamp = msg->timestamp_ns; 
        order.last_update_time = order.timestamp;

        order.message_type = msg->message_type;
        order.protocol = Protocol::ITCH;
        order.order_type = (order.price < 0) ? OrderType::Limit : OrderType::Market;
        order.cancelled_count = 0;
    }

    inline void fill_itch_exec_report(Order &order, const BIST::ITCHOrderExecutedMessage *msg) noexcept
    {
        order.filled_quantity += msg->executed_quantity;
        order.last_exec_quantity = msg->executed_quantity;
        order.last_update_time = msg->timestamp_ns;
        if (order.filled_quantity < order.quantity)
            order.status = Status::Partial;
        else
            order.status = Status::Filled;
    }

    inline void fill_itch_exec_report(Order &order, const BIST::ITCHOrderExecutedWithPriceMessage *msg) noexcept
    {
        fill_itch_exec_report(order, reinterpret_cast<const BIST::ITCHOrderExecutedMessage *>(msg));
        order.price = static_cast<int64_t>(msg->trade_price);
    }

    inline void fill_itch_delete(Order &order, const BIST::ITCHOrderDeleteMessage *msg) noexcept
    {
        order.status = Status::Cancelled;
        order.remaining_quantity = order.quantity;
        order.last_update_time = msg->timestamp_ns;
        order.cancelled_count++;
    }

    inline void fill_itch_trade(Order &order, const BIST::ITCHTradeMessage *msg, Venue venue) noexcept
    {
        if (UNLIKELY(order.isOurOrder))
            return;

        auto &instrument_map = instrument_cache_[std::underlying_type_t<Venue>(venue)];

        order.order_id = ++bist_trade_id; // BIST trade mesajlarında order_id yok, trade_id olarak artan bir değer kullanıyoruz
        order.filled_quantity += msg->quantity;
        order.last_exec_quantity = msg->quantity;
        order.last_update_time = msg->timestamp_ns;
        order.instrument_id = msg->order_book_id;

        auto it = instrument_map.find(order.instrument_id);
        if (LIKELY(it != instrument_map.end()))
        {
            order.symbol = it->second.symbol;
            order.symbol_index = hashtables_.getIndex(static_cast<uint8_t>(venue), order.symbol);
        }

        order.price = static_cast<int64_t>(msg->trade_price);
        order.status = Status::Filled;
        if(msg->side == ' ')
            order.side = Side::Unknown;
        else 
            order.side = (msg->side == 'B') ? Side::Buy : Side::Sell;
    }


    //===========================================================
    //=================== NASDAQ ITCH fillers ===================
    //=========================================================== 
    inline void fill_itch_add(Order &order, const NASDAQ::ITCHAddOrderMessage *msg, Venue venue) noexcept
    {
        auto &instrument_map = instrument_cache_[std::underlying_type_t<Venue>(venue)];
        
        order.price = static_cast<int64_t>(msg->price);
        order.quantity = msg->shares;
        order.filled_quantity = 0;
        order.instrument_id = msg->stock_locate;

        auto it = instrument_map.find(order.instrument_id);
        if (LIKELY(it != instrument_map.end()))
        {
            order.symbol = it->second.symbol;
            order.symbol_index = hashtables_.getIndex(static_cast<uint8_t>(venue),order.symbol);
        }

        order.side = (msg->side == 'B') ? Side::Buy : Side::Sell;
        order.status = Status::New;

        order.order_id = msg->order_ref;
        order.timestamp = msg->timestamp; // nanosaniyeyi saniyeye çevir
        order.last_update_time = order.timestamp;

        order.message_type = msg->message_type;
        order.protocol = Protocol::ITCH;
        order.order_type = (order.price < 0) ? OrderType::Limit : OrderType::Market;
        order.cancelled_count = 0;
        // order.timestamp_ns = msg->timestamp;
    }

    inline void fill_itch_add(Order &order, const NASDAQ::ITCHAddOrderMPIDMessage *msg, Venue venue) noexcept
    {
        auto &instrument_map = instrument_cache_[std::underlying_type_t<Venue>(venue)];

        order.price = static_cast<int64_t>(msg->price);
        order.quantity = msg->shares;
        order.filled_quantity = 0;
        order.instrument_id = msg->stock_locate;
        copy_symbol(order.symbol, msg->stock);
        order.symbol_index = hashtables_.getIndex(static_cast<uint8_t>(venue),order.symbol);
        
        order.side = (msg->side == 'B') ? Side::Buy : Side::Sell;
        order.status = Status::New;

        order.order_id = msg->order_ref;
        order.timestamp = msg->timestamp;
        order.last_update_time = order.timestamp;

        order.message_type = msg->message_type;
        order.protocol = Protocol::ITCH;
        order.order_type = (order.price < 0) ? OrderType::Limit : OrderType::Market;
        order.cancelled_count = 0;

        // order.timestamp_ns = msg->timestamp;
        // MPID alanı order struct'ta yok, gerekirse eklenebilir
    }

    inline void fill_itch_cancel(Order &order, const NASDAQ::ITCHCancelMessage *msg) noexcept
    {
        order.status = Status::Cancelled;
        order.last_update_time = msg->timestamp;
        order.remaining_quantity = msg->cancelled_shares;
        order.cancelled_count++;
    }

    inline void fill_itch_exec_report(Order &order, const NASDAQ::ITCHExecutedMessage *msg) noexcept
    {
        order.filled_quantity += msg->executed_shares;
        order.last_exec_quantity = msg->executed_shares;
        order.last_update_time = msg->timestamp;
        if (order.filled_quantity < order.quantity)
            order.status = Status::Partial;
        else
            order.status = Status::Filled;
    }

    inline void fill_itch_exec_report(Order &order, const NASDAQ::ITCHExecutedWithPriceMessage *msg) noexcept
    {
        fill_itch_exec_report(order, reinterpret_cast<const NASDAQ::ITCHExecutedMessage *>(msg));
        order.price = static_cast<int64_t>(msg->execution_price);
    }

    inline void fill_itch_delete(Order &order, const NASDAQ::ITCHDeleteMessage *msg) noexcept
    {
        order.status = Status::Cancelled;
        order.remaining_quantity = order.quantity - order.filled_quantity;
        order.last_update_time = msg->timestamp;
    }

    inline void fill_itch_replace(Order &order, const NASDAQ::ITCHReplaceMessage *msg, Venue venue) noexcept 
    {
        auto &instrument_map = instrument_cache_[std::underlying_type_t<Venue>(venue)];

        order.order_id = msg->new_order_ref;
        order.price = msg->price;
        order.quantity = msg->shares;
        order.instrument_id = msg->stock_locate;

        auto it = instrument_map.find(order.instrument_id);
        if (LIKELY(it != instrument_map.end()))
        {
            order.symbol = it->second.symbol;
            order.symbol_index = hashtables_.getIndex(static_cast<uint8_t>(venue), order.symbol);
        }

        order.message_type = msg->message_type;
        order.status = Status::New;
        order.protocol = Protocol::ITCH;
        order.last_update_time = msg->timestamp;
        order.order_type = (order.price < 0) ? OrderType::Limit : OrderType::Market;
        order.cancelled_count = 0;
    }

    inline void fill_itch_trade(Order &order, const NASDAQ::ITCHTradeMessage *msg, Venue venue) noexcept
    {
        if (UNLIKELY(order.isOurOrder))
            return;
        
        auto &instrument_map = instrument_cache_[std::underlying_type_t<Venue>(venue)];

        order.order_id = msg->order_ref; 
        order.filled_quantity += msg->shares;
        order.last_exec_quantity = msg->shares;
        order.last_update_time = msg->timestamp;
        order.instrument_id = msg->stock_locate;

        auto it = instrument_map.find(order.instrument_id);
        if (LIKELY(it != instrument_map.end()))
        {
            order.symbol = it->second.symbol;
            order.symbol_index = hashtables_.getIndex(static_cast<uint8_t>(venue),order.symbol);
        }

        order.price = static_cast<int64_t>(msg->price);
        order.status = Status::Filled;
    }
    
    //===========================================================
    //======================= SBE fillers =======================
    //===========================================================
    inline void fill_sbe_add(Order &order, const SBEAddOrderMessage *msg, Venue venue) noexcept
    {
        auto &instrument_map = instrument_cache_[std::underlying_type_t<Venue>(venue)];

        order.price = static_cast<int64_t>(msg->price);
        order.quantity = msg->quantity;
        order.side = (msg->side == 0) ? Side::Buy : Side::Sell;
        order.status = Status::New;
        order.instrument_id = msg->header.schemaId; // veya header.version kullanılabilir

        auto it = instrument_map.find(order.instrument_id);
        if (LIKELY(it != instrument_map.end()))
        {
            order.symbol = it->second.symbol;
            order.symbol_index = hashtables_.getIndex(static_cast<uint8_t>(venue),order.symbol);
        }

        order.timestamp = msg->header.version;
        order.last_update_time = order.timestamp;

        order.message_type = static_cast<uint8_t>(msg->header.templateId);
        order.protocol = Protocol::SBE;
        order.order_type = (order.price < 0) ? OrderType::Limit : OrderType::Market;
        order.cancelled_count = 0;
        // order.timestamp_ns = msg->header.version;
    }

    inline void fill_sbe_modify(Order &order, const SBEModifyOrderMessage *msg) noexcept
    {
        order.quantity = msg->newQuantity;
        order.last_update_time = msg->header.version;
    }

    inline void fill_sbe_delete(Order &order, const SBEDeleteOrderMessage *msg) noexcept
    {
        order.status = Status::Cancelled;
        order.remaining_quantity = order.quantity - order.filled_quantity;
        order.last_update_time = msg->header.version;
        order.cancelled_count++;
    }

    inline void fill_sbe_trade(Order &order, const SBETradeMessage *msg, Venue venue) noexcept
    {
        if (UNLIKELY(order.isOurOrder))
            return;
    
        auto &instrument_map = instrument_cache_[std::underlying_type_t<Venue>(venue)];

        order.price = static_cast<int64_t>(msg->price);
        order.filled_quantity += msg->quantity;
        order.last_exec_quantity = msg->quantity;
        order.instrument_id = msg->header.schemaId;

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

        order.last_update_time = msg->header.version;
        order.message_type = static_cast<uint8_t>(msg->header.templateId);
        order.protocol = Protocol::SBE;
        order.cancelled_count = 0;
    }
};
