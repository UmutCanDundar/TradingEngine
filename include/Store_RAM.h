
#pragma once

#include "Protocol-Venue.h"
#include "Parser_Dispatch.h"
#include "common.h"

#include <cstdint>
#include <array>
#include <absl/container/flat_hash_map.h>
// #include <absl/container/flat_hash_set.h>
#include <absl/hash/hash.h>
#include <boost/lockfree/spsc_queue.hpp>
#include <vector>
#include <cstring>
#include <immintrin.h>
#include <bit>
#include <tuple>

enum class Side : uint8_t
{
    Unknown = 0,
    Buy = 1,
    Sell = 2
};
enum class Status : uint8_t
{
    // === BASIC STATES ===
    Unknown = 0,   // İlk state, henüz bilgi yok
    New = 1,       // Order kabul edildi, market'ta aktif
    Partial = 2,   // Kısmen doldu, hala aktif
    Filled = 3,    // Tamamen doldu
    Cancelled = 4, // İptal edildi

    // === PENDING STATES ===
    PendingNew = 10,     // Order gönderildi, onay bekleniyor
    PendingCancel = 11,  // Cancel gönderildi, onay bekleniyor
    PendingReplace = 12, // Modify gönderildi, onay bekleniyor

    // === REJECTION STATES ===
    Rejected = 20,      // Order reddedildi
    CancelReject = 21,  // Cancel reddedildi
    ReplaceReject = 22, // Modify reddedildi

    // === EXCHANGE SPECIFIC ===
    Suspended = 30,                 // Trading halted
    Expired = 31,                   // TTL süresi doldu
    PartiallyFilled_Cancelled = 32, // Kısmen dolup iptal edildi

    // === ERROR STATES ===
    DoneForDay = 40, // Gün sonu kapandı
    Calculated = 41, // Hesaplanmış ama henüz aktif değil
    Stopped = 42,    // Stop order tetiklendi

    // === SYSTEM STATES ===
    PendingSubmit = 50, // Sistem tarafında bekliyor
    Accepted = 51,      // Exchange kabul etti
    Restated = 52       // Order yeniden tanımlandı
};

struct alignas(64) Order
{
    // 🔴 HOT PATH (en sık erişilen alanlar)
    int64_t price = 0;            // Fixed-point scaled
    uint32_t quantity = 0;        // Original order quantity
    uint32_t filled_quantity = 0; // Cumulative filled quantity
    uint32_t cancelled_quantity = 0;
    std::array<char, 8> symbol{};    // Fixed-size symbol for low-latency lookup
    Side side = Side::Unknown;       // Buy/Sell
    Status status = Status::Unknown; // New/Partial/Filled/Cancelled
    bool isOurOrder = false;

    // 🟠 LOOKUP & ROUTING
    uint8_t pad1[1];               // 64-byte alignment
    uint64_t order_id = 0;         // Exchange-assigned unique order ID
    uint64_t client_order_id = 0;  // Strategy-assigned client order ID
    uint64_t timestamp = 0;        // Order creation time (ns epoch)
    uint64_t last_update_time = 0; // Last modification time (ns epoch)

    // 🟡 PROTOCOL METADATA
    uint16_t message_type = 0;         // FIX char, ITCH char, SBE templateId
    Protocol protocol = Protocol::FIX; // Source protocol for reconstruction

    // 🟢 OPTIONAL (protokol bazlı karar & advanced tactics)
    Venue venue;                // NYSE, NASDAQ, etc.
    uint32_t instrument_id = 0; // SBE/ITCH
    uint8_t time_in_force = 0;  // IOC, GTC, etc.
    uint8_t order_type = 0;     // Limit, Market, Stop
    uint8_t priority_level = 0; // HFT queue tactics

    uint8_t pad3[53]; // 64-byte alignment
};

struct SymbolMeta
{
    std::array<char, 8> symbol;
    uint32_t lot_size = 0;
    uint32_t tick_size = 0;
};

struct OrderKey
{
    uint64_t order_id;
    Protocol protocol;
    Venue venue;

    bool operator==(const OrderKey &other) const
    {
        return order_id == other.order_id && protocol == other.protocol && venue == other.venue;
    }
};

struct OrderKeyHash
{
    size_t operator()(const OrderKey &k) const noexcept
    {
        return absl::Hash<std::tuple<Venue, Protocol, uint64_t>>{}(
            std::make_tuple(k.venue, k.protocol, k.order_id));
    }
};

inline constexpr size_t ORDER_QUEUE_CAPACITY = 65536;

using spscOrderQueue_t = boost::lockfree::spsc_queue<Order *, boost::lockfree::capacity<ORDER_QUEUE_CAPACITY>>;
using spscDbQueue_t = boost::lockfree::spsc_queue<std::variant<Order *, MessageWithVenue<FIXMessage *>, MessageWithVenue<ITCHMessage>, MessageWithVenue<SBEMessage>>, boost::lockfree::capacity<ORDER_QUEUE_CAPACITY>>;

class Store_RAM
{
private:
    static constexpr size_t ORDER_POOL_CAPACITY = 65536; // 8MB @128B each
    static constexpr size_t MARKETORDER_LAST_INDEX = 32768;
    static constexpr size_t ORDER_MAP_CAPACITY = 32768;

    std::vector<Order> order_pool_;
    size_t market_next_slot = 0;
    size_t our_next_slot = MARKETORDER_LAST_INDEX;

    absl::flat_hash_map<OrderKey, Order *, OrderKeyHash> market_order_map_;
    absl::flat_hash_map<OrderKey, Order *, OrderKeyHash> our_order_map_;
    absl::flat_hash_map<uint64_t, SymbolMeta> instrument_cache_;

    spscMessageQueue_t &parser_to_store_;
    spscOrderQueue_t &store_to_strategy_;
    spscOrderQueue_t &store_to_strategy_free_slot_;
    spscDbQueue_t &store_to_db_;

public:
    Store_RAM(spscMessageQueue_t &parser_to_store,
              spscOrderQueue_t &store_to_strategy,
              spscOrderQueue_t &store_to_strategy_free_slot,
              spscDbQueue_t &store_to_db) noexcept;

    inline Order *poll_update() noexcept
    {
        Order *order = nullptr;
        if (store_to_strategy_.pop(order))
            return order;
        return nullptr;
    }

    template <typename T>
    inline void store(const MessageWithVenue<T> &msg) noexcept
    {
        this->update_order(msg);
    }
    void store() noexcept;

    Order *add_our_order(uint64_t order_id, Protocol protocol, Venue venue) noexcept;

private:
    Order *add_market_order(uint64_t order_id, Protocol protocol, Venue venue) noexcept;
    Order *get_order(uint64_t order_id, Protocol protocol, Venue venue) noexcept;

    void update_order(const MessageWithVenue<FIXMessage *> &fixMsg) noexcept;
    void update_order(const MessageWithVenue<ITCHMessage> &itchMsg) noexcept;
    void update_order(const MessageWithVenue<SBEMessage> &sbeMsg) noexcept;

    void handle_instrument_definition(const SBEInstrumentDefinitionMessage *msg) noexcept;

    inline void ReleaseOrder(OrderKey &&orderkey)
    {
        store_to_strategy_free_slot_.push(our_order_map_[orderkey]);
        our_order_map_.erase(orderkey);
    }

    static inline void copy_symbol(std::array<char, 8> &dest, std::string_view src) noexcept
    {
        std::memcpy(dest.data(), src.data(), 8);

        for (size_t i = 0; i < 8; ++i)
        {
            if (dest[i] == ' ' || dest[i] == '\0')
            {
                std::memset(dest.data() + i, 0, 8 - i);
                break;
            }
        }
    }

    // ===== FIX fillers =====
    inline void fill_fix_new(Order &order, const FIXMessage *msg) noexcept
    {
        order.price = msg->price;
        order.quantity = msg->quantity;
        order.filled_quantity = 0; // Yeni emir, henüz doldurulmamış
        copy_symbol(order.symbol, msg->symbol);
        order.side = (msg->side == '1') ? Side::Buy : Side::Sell; // FIX: '1'=Buy, '2'=Sell
        order.status = Status::New;

        order.order_id = absl::Hash<std::string_view>{}(msg->order_id);
        order.client_order_id = absl::Hash<std::string_view>{}(msg->cl_ord_id);

        order.timestamp = static_cast<uint64_t>(msg->transact_time);
        order.last_update_time = order.timestamp;

        order.message_type = static_cast<uint16_t>(msg->msg_type);
        order.protocol = Protocol::FIX;

        order.time_in_force = static_cast<uint8_t>(msg->time_in_force);
        order.order_type = static_cast<uint8_t>(msg->ord_type);
        // order.timestamp_ns = static_cast<uint64_t>(msg->transact_time) * 1'000'000'000ULL;
        // Diğer opsiyonel alanlar sıfır kalabilir
    }

    inline void fill_fix_cancel(Order &order, const FIXMessage *msg) noexcept
    {
        order.status = Status::Cancelled;
        order.last_update_time = static_cast<uint64_t>(msg->transact_time);
    }

    inline void fill_fix_modify(Order &order, const FIXMessage *msg) noexcept
    {
        order.price = static_cast<uint64_t>(msg->price);
        order.quantity = msg->quantity;
        order.time_in_force = static_cast<uint8_t>(msg->time_in_force);
        order.last_update_time = static_cast<uint64_t>(msg->transact_time);
    }

    void fill_fix_exec_report(Order &order, const FIXMessage *msg) noexcept;

    inline void fill_fix_cancel_reject(Order &order, const FIXMessage *msg) noexcept
    {
        // Cancel reject’te status değişmez, sadece zaman güncellenebilir(bakılacak?)
        order.status = Status::CancelReject;
        order.last_update_time = static_cast<uint64_t>(msg->transact_time);
    }

    // ===== ITCH fillers =====
    inline void fill_itch_add(Order &order, const ITCHAddOrderMessage *msg) noexcept
    {
        order.price = static_cast<int64_t>(msg->price);
        order.quantity = msg->quantity;
        order.filled_quantity = 0;
        copy_symbol(order.symbol, msg->stock);
        order.side = (msg->side == 'B') ? Side::Buy : Side::Sell;
        order.status = Status::New;

        order.order_id = msg->order_ref;
        order.timestamp = msg->timestamp / 1000000000ULL; // nanosaniyeyi saniyeye çevir
        order.last_update_time = order.timestamp;

        order.message_type = static_cast<uint16_t>(msg->message_type);
        order.protocol = Protocol::ITCH;

        // order.timestamp_ns = msg->timestamp;
    }

    inline void fill_itch_add(Order &order, const ITCHAddOrderMPIDMessage *msg) noexcept
    {
        order.price = static_cast<int64_t>(msg->price);
        order.quantity = msg->quantity;
        order.filled_quantity = 0;
        copy_symbol(order.symbol, msg->stock);
        order.side = (msg->side == 'B') ? Side::Buy : Side::Sell;
        order.status = Status::New;

        order.order_id = msg->order_ref;
        order.timestamp = msg->timestamp / 1000000000ULL;
        order.last_update_time = order.timestamp;

        order.message_type = static_cast<uint16_t>(msg->message_type);
        order.protocol = Protocol::ITCH;

        // order.timestamp_ns = msg->timestamp;
        // MPID alanı order struct'ta yok, gerekirse eklenebilir
    }

    inline void fill_itch_cancel(Order &order, const ITCHCancelMessage *msg) noexcept
    {
        if (order.filled_quantity > 0)
            order.status = Status::PartiallyFilled_Cancelled;
        else
            order.status = Status::Cancelled;

        order.last_update_time = msg->timestamp / 1000000000ULL;
        order.cancelled_quantity = msg->cancelled_quantity;
    }

    inline void fill_itch_exec_report(Order &order, const ITCHExecutedMessage *msg) noexcept
    {
        order.filled_quantity += msg->executed_quantity;
        order.last_update_time = msg->timestamp / 1000000000ULL;
        if (order.filled_quantity < order.quantity)
            order.status = Status::Partial;
        else
            order.status = Status::Filled;
    }

    inline void fill_itch_exec_report(Order &order, const ITCHExecutedWithPriceMessage *msg) noexcept
    {
        fill_itch_exec_report(order, reinterpret_cast<const ITCHExecutedMessage *>(msg));
        order.price = static_cast<int64_t>(msg->execution_price);
    }

    inline void fill_itch_delete(Order &order, const ITCHDeleteMessage *msg) noexcept
    {
        if (order.filled_quantity > 0)
            order.status = Status::PartiallyFilled_Cancelled;
        else
            order.status = Status::Cancelled;
        order.last_update_time = msg->timestamp / 1000000000ULL;
    }

    inline void fill_itch_trade(Order &order, const ITCHTradeMessage *msg) noexcept
    {
        order.filled_quantity += msg->quantity;
        order.last_update_time = msg->timestamp / 1000000000ULL;
        copy_symbol(order.symbol, msg->stock);
        order.price = static_cast<int64_t>(msg->price);
        // Genelde trade mesajları yeni emir yaratmaz, sadece execution bilgisi verir
    }

    // ===== SBE fillers =====
    inline void fill_sbe_add(Order &order, const SBEAddOrderMessage *msg) noexcept
    {
        order.price = static_cast<int64_t>(msg->price);
        order.quantity = msg->quantity;
        order.side = (msg->side == 0) ? Side::Buy : Side::Sell;
        order.status = Status::New;

        order.instrument_id = msg->header.schemaId; // veya header.version kullanılabilir
        auto it = instrument_cache_.find(order.instrument_id);
        if (it != instrument_cache_.end())
        {
            order.symbol = it->second.symbol;
        }

        order.timestamp = msg->header.version / 1'000'000'000ULL;
        order.last_update_time = order.timestamp;

        order.message_type = msg->header.templateId;
        order.protocol = Protocol::SBE;

        // order.timestamp_ns = msg->header.version;
    }

    inline void fill_sbe_modify(Order &order, const SBEModifyOrderMessage *msg) noexcept
    {
        order.quantity = msg->newQuantity;
        order.last_update_time = msg->header.version / 1'000'000'000ULL;
    }

    inline void fill_sbe_cancel(Order &order, const SBEDeleteOrderMessage *msg) noexcept
    {
        if (order.filled_quantity > 0)
            order.status = Status::PartiallyFilled_Cancelled;
        else
            order.status = Status::Cancelled;

        order.last_update_time = msg->header.version / 1'000'000'000ULL;
    }

    inline void fill_sbe_trade(Order &order, const SBETradeMessage *msg) noexcept
    {
        order.price = static_cast<int64_t>(msg->price);
        order.filled_quantity += msg->quantity;
        order.instrument_id = msg->header.schemaId;

        if (order.filled_quantity < order.quantity)
            order.status = Status::Partial;
        else
            order.status = Status::Filled;

        auto it = instrument_cache_.find(order.instrument_id);
        if (it != instrument_cache_.end())
        {
            order.symbol = it->second.symbol;
        }

        order.last_update_time = msg->header.version / 1'000'000'000ULL;
        order.message_type = msg->header.templateId;
        order.protocol = Protocol::SBE;
    }
};
