
#pragma once

#include "Protocol-Venue.h"
#include "Parser.h"
#include "common.h"

#include <cstdint>
#include <array>
#include <absl/container/flat_hash_map.h>
#include <absl/hash/hash.h>
#include <boost/lockfree/spsc_queue.hpp>
#include <vector>
#include <cstring>
#include <immintrin.h>
#include <bit>

enum class Side : uint8_t
{
    Buy = 0,
    Sell = 1
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
    std::array<char, 8> symbol{}; // Fixed-size symbol for low-latency lookup
    Side side = Side::Buy;        // Buy/Sell
    Status status = Status::New;  // New/Partial/Filled/Cancelled

    // 🟠 LOOKUP & ROUTING
    uint8_t pad1[2];               // 64-byte alignment
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

class Store
{
private:
    static constexpr size_t ORDER_POOL_SIZE = 65536; // 8MB @128B each
    static constexpr size_t QUEUE_SIZE = 65536;

    alignas(64) std::vector<Order> order_pool_;
    size_t next_slot = 0;

    absl::flat_hash_map<OrderKey, Order *, OrderKeyHash> order_map_;
    absl::flat_hash_map<uint64_t, SymbolMeta> instrument_cache_;

    boost::lockfree::spsc_queue<Order *, boost::lockfree::capacity<QUEUE_SIZE>> update_queue;

    inline void copy_symbol(std::array<char, 8> &dest, std::string_view src) noexcept
    {
        size_t len = std::size(src) > 8 ? 8 : src.size();
        std::memcpy(dest.data(), src.data(), len);
        if (len < 8)
            std::memset(dest.data() + len, 0, 8 - len);
    }

    void handle_instrument_definition(const SBEInstrumentDefinitionMessage &msg) noexcept
    {
        SymbolMeta meta;
        std::string_view symbol_src(reinterpret_cast<const char *>(msg.currencyCode), 3);
        copy_symbol(meta.symbol, symbol_src);
        meta.lot_size = msg.lotSize;
        meta.tick_size = 1; // Eğer feed veriyorsa gerçek tick_size kullanılır

        instrument_cache_[msg.instrumentId] = meta;
    }

public:
    Store()
    {
        order_pool_.resize(ORDER_POOL_SIZE);
        order_map_.reserve(ORDER_POOL_SIZE);
    }

    Order *poll_update() noexcept
    {
        Order *order = nullptr;
        if (update_queue.pop(order))
            return order;
        return nullptr;
    }

    template <typename T>
    void dispatch_update_order(const MessageWithVenue<T> &msg) noexcept
    {
        this->update_order(msg);
    }

    void dispatch_update_order(const MessageWithVenue<std::variant<FIXMessage, ITCHMessage, SBEMessage>> &msgWithVenue) noexcept
    {
        std::visit([this, &msgWithVenue](const auto &inner_msg)
                   {
            using MsgType = std::decay_t<decltype(inner_msg)>;
            this->dispatch_update_order(MessageWithVenue<MsgType>{inner_msg, msgWithVenue.venue}); }, msgWithVenue.msg);
    }

private:
    Order *add_order(uint64_t order_id, Protocol protocol, Venue venue) noexcept
    {
        if (UNLIKELY(next_slot >= ORDER_POOL_SIZE))
            return nullptr;

        Order *order = &order_pool_[next_slot++];
        order_map_[OrderKey{order_id, protocol, venue}] = order;
        return order;
    }

    Order *get_order(uint64_t order_id, Protocol protocol, Venue venue) noexcept
    {
        auto it = order_map_.find(OrderKey{order_id, protocol, venue});
        return (it != order_map_.end()) ? it->second : nullptr;
    }

    // ================= FIX Handler ===================
    void update_order(const MessageWithVenue<FIXMessage> &fixMsg) noexcept
    {
        auto &msg = fixMsg.msg;

        alignas(64) constexpr auto allowed_exec_type = []
        {
            std::array<uint8_t, 128> arr = {};
            arr['0'] = 1;
            arr['1'] = 1;
            arr['2'] = 1;
            arr['4'] = 1;
            arr['8'] = 1;
            arr['6'] = 1;
            arr['5'] = 1;
            arr['E'] = 1; // 69
            arr['C'] = 1;
            return arr;
        }();

        uint64_t order_id = absl::Hash<std::string_view>{}(msg.cl_ord_id);
        Order *order = get_order(order_id, Protocol::FIX, fixMsg.venue);
        if (!order)
        {
            order = add_order(order_id, Protocol::FIX, fixMsg.venue);
            if (UNLIKELY(!order))
                return;
        }

        switch (msg.msg_type)
        {
        case 'D': // NewOrderSingle
            fill_fix_new(*order, msg);
            break;
        case 'F': // CancelRequest
            fill_fix_cancel(*order, msg);
            break;
        case 'G': // ModifyRequest
            fill_fix_modify(*order, msg);
            break;
        case '8': // ExecutionReport
            if (allowed_exec_type[static_cast<uint8_t>(msg.exec_type)])
                fill_fix_exec_report(*order, msg);
            else
                return;
            break;
        case '9': // OrderCancelReject
            fill_fix_cancel_reject(*order, msg);
            break;
        }

        update_queue.push(order);
    }

    // ================= ITCH Handler ===================
    void update_order(const MessageWithVenue<ITCHMessage> &itchMsg) noexcept
    {
        std::visit([this, venue = itchMsg.venue](const auto &msg)
                   {
            if constexpr (requires { msg.order_ref; }) 
            {
                uint64_t order_id = msg.order_ref;
                Order *order = this->get_order(order_id, Protocol::ITCH, venue);
                if (!order)
                {
                    order = this->add_order(order_id, Protocol::ITCH, venue);
                    if (UNLIKELY(!order))
                        return;
                }

                using MsgType = std::decay_t<decltype(msg)>;

                if constexpr (std::is_same_v<MsgType, ITCHAddOrderMessage> ||
                            std::is_same_v<MsgType, ITCHAddOrderMPIDMessage>)
                {
                    this->fill_itch_add(*order, msg);
                }
                else if constexpr (std::is_same_v<MsgType, ITCHCancelMessage>)
                {
                    this->fill_itch_cancel(*order, msg);
                }
                else if constexpr (std::is_same_v<MsgType, ITCHExecutedMessage> ||
                                std::is_same_v<MsgType, ITCHExecutedWithPriceMessage>)
                {
                    this->fill_itch_exec_report(*order, msg);
                }
                else if constexpr (std::is_same_v<MsgType, ITCHDeleteMessage>)
                {
                    this->fill_itch_delete(*order, msg);
                }
                else if constexpr (std::is_same_v<MsgType, ITCHTradeMessage>)
                {
                    this->fill_itch_trade(*order, msg);
                }

                this->update_queue.push(order);
            } }, itchMsg.msg);
    }

    // ================= SBE Handler ===================
    void update_order(const MessageWithVenue<SBEMessage> &sbeMsg) noexcept
    {
        std::visit([this, venue = sbeMsg.venue](const auto &msg)
                   {

            using MsgType = std::decay_t<decltype(msg)>;

            if constexpr (std::is_same_v<MsgType, SBEAddOrderMessage> ||
                            std::is_same_v<MsgType, SBEModifyOrderMessage> ||
                            std::is_same_v<MsgType, SBEDeleteOrderMessage> ||
                            std::is_same_v<MsgType, SBETradeMessage>)
            {
                uint64_t order_id = msg.orderId;
                Order *order = this->get_order(order_id, Protocol::SBE, venue);
                if (!order)
                {
                    order = this->add_order(order_id, Protocol::SBE, venue);
                    if (UNLIKELY(!order))
                        return;
                }

                if constexpr (std::is_same_v<MsgType, SBEAddOrderMessage>)
                {
                    this->fill_sbe_add(*order, msg);
                }
                else if constexpr (std::is_same_v<MsgType, SBEModifyOrderMessage>)
                {
                    this->fill_sbe_modify(*order, msg);
                }
                else if constexpr (std::is_same_v<MsgType, SBEDeleteOrderMessage>)
                {
                    this->fill_sbe_cancel(*order, msg);
                }
                else if constexpr (std::is_same_v<MsgType, SBETradeMessage>)
                {
                    this->fill_sbe_trade(*order, msg);
                }

                this->update_queue.push(order);
            }
            else if constexpr (std::is_same_v<MsgType, SBEInstrumentDefinitionMessage>)
            {
                this->handle_instrument_definition(msg);
            } }, sbeMsg.msg);
    }

    // ===== FIX fillers =====
    inline void fill_fix_new(Order &order, const FIXMessage &msg) noexcept
    {
        order.price = msg.price;
        order.quantity = msg.quantity;
        order.filled_quantity = 0; // Yeni emir, henüz doldurulmamış
        copy_symbol(order.symbol, msg.symbol);
        order.side = (msg.side == '1') ? Side::Buy : Side::Sell; // FIX: '1'=Buy, '2'=Sell
        order.status = Status::New;

        order.order_id = absl::Hash<std::string_view>{}(msg.order_id);
        order.client_order_id = absl::Hash<std::string_view>{}(msg.cl_ord_id);

        order.timestamp = static_cast<uint64_t>(msg.transact_time);
        order.last_update_time = order.timestamp;

        order.message_type = static_cast<uint16_t>(msg.msg_type);
        order.protocol = Protocol::FIX;

        order.time_in_force = static_cast<uint8_t>(msg.time_in_force);
        order.order_type = static_cast<uint8_t>(msg.ord_type);
        // order.timestamp_ns = static_cast<uint64_t>(msg.transact_time) * 1'000'000'000ULL;
        // Diğer opsiyonel alanlar sıfır kalabilir
    }

    inline void fill_fix_cancel(Order &order, const FIXMessage &msg) noexcept
    {
        order.status = Status::Cancelled;
        order.last_update_time = static_cast<uint64_t>(msg.transact_time);
    }

    inline void fill_fix_modify(Order &order, const FIXMessage &msg) noexcept
    {
        order.price = static_cast<uint64_t>(msg.price);
        order.quantity = msg.quantity;
        order.time_in_force = static_cast<uint8_t>(msg.time_in_force);
        order.last_update_time = static_cast<uint64_t>(msg.transact_time);
    }

    inline void fill_fix_exec_report(Order &order, const FIXMessage &msg) noexcept
    {
        switch (msg.ord_status)
        {
        case '0': // New
            order.status = Status::New;
            break;
        case '1': // Partially Filled
            order.status = Status::Partial;
            break;
        case '2': // Filled
            order.status = Status::Filled;
            break;
        case '4': // Cancelled
            if (order.filled_quantity > 0)
                order.status = Status::PartiallyFilled_Cancelled;
            else
                order.status = Status::Cancelled;
            break;
        case '8': // Rejected
            order.status = Status::Rejected;
            break;
        case '6': // Pending Cancel
            order.status = Status::PendingCancel;
            break;
        case '5':                       // Replaced
            order.status = Status::New; // Active again after modification
            break;
        case 'E': // Pending Replace
            order.status = Status::PendingReplace;
            break;
        case 'C': // Expired
            if (order.filled_quantity > 0)
                order.status = Status::PartiallyFilled_Cancelled;
            else
                order.status = Status::Expired;
            break;
        }

        order.filled_quantity = msg.filled_qty;
        order.quantity = msg.leaves_qty + msg.filled_qty;
        order.last_update_time = static_cast<uint64_t>(msg.transact_time);
    }

    inline void fill_fix_cancel_reject(Order &order, const FIXMessage &msg) noexcept
    {
        // Cancel reject’te status değişmez, sadece zaman güncellenebilir(bakılacak?)
        order.status = Status::CancelReject;
        order.last_update_time = static_cast<uint64_t>(msg.transact_time);
    }

    // ===== ITCH fillers =====
    inline void fill_itch_add(Order &order, const ITCHAddOrderMessage &msg) noexcept
    {
        order.price = static_cast<int64_t>(msg.price);
        order.quantity = msg.quantity;
        order.filled_quantity = 0;
        copy_symbol(order.symbol, msg.stock);
        order.side = (msg.side == 'B') ? Side::Buy : Side::Sell;
        order.status = Status::New;

        order.order_id = msg.order_ref;
        order.timestamp = msg.timestamp / 1000000000ULL; // nanosaniyeyi saniyeye çevir
        order.last_update_time = order.timestamp;

        order.message_type = static_cast<uint16_t>(msg.message_type);
        order.protocol = Protocol::ITCH;

        // order.timestamp_ns = msg.timestamp;
    }

    inline void fill_itch_add(Order &order, const ITCHAddOrderMPIDMessage &msg) noexcept
    {
        order.price = static_cast<int64_t>(msg.price);
        order.quantity = msg.quantity;
        order.filled_quantity = 0;
        copy_symbol(order.symbol, msg.stock);
        order.side = (msg.side == 'B') ? Side::Buy : Side::Sell;
        order.status = Status::New;

        order.order_id = msg.order_ref;
        order.timestamp = msg.timestamp / 1000000000ULL;
        order.last_update_time = order.timestamp;

        order.message_type = static_cast<uint16_t>(msg.message_type);
        order.protocol = Protocol::ITCH;

        // order.timestamp_ns = msg.timestamp;
        // MPID alanı order struct'ta yok, gerekirse eklenebilir
    }

    inline void fill_itch_cancel(Order &order, const ITCHCancelMessage &msg) noexcept
    {
        if (order.filled_quantity > 0)
            order.status = Status::PartiallyFilled_Cancelled;
        else
            order.status = Status::Cancelled;

        order.last_update_time = msg.timestamp / 1000000000ULL;
        order.cancelled_quantity = msg.cancelled_quantity;
    }

    inline void fill_itch_exec_report(Order &order, const ITCHExecutedMessage &msg) noexcept
    {
        order.filled_quantity += msg.executed_quantity;
        order.last_update_time = msg.timestamp / 1000000000ULL;
        if (order.filled_quantity < order.quantity)
            order.status = Status::Partial;
        else
            order.status = Status::Filled;
    }

    inline void fill_itch_exec_report(Order &order, const ITCHExecutedWithPriceMessage &msg) noexcept
    {
        fill_itch_exec_report(order, reinterpret_cast<const ITCHExecutedMessage &>(msg));
        order.price = static_cast<int64_t>(msg.execution_price);
    }

    inline void fill_itch_delete(Order &order, const ITCHDeleteMessage &msg) noexcept
    {
        if (order.filled_quantity > 0)
            order.status = Status::PartiallyFilled_Cancelled;
        else
            order.status = Status::Cancelled;
        order.last_update_time = msg.timestamp / 1000000000ULL;
    }

    inline void fill_itch_trade(Order &order, const ITCHTradeMessage &msg) noexcept
    {
        order.filled_quantity += msg.quantity;
        order.last_update_time = msg.timestamp / 1000000000ULL;
        copy_symbol(order.symbol, msg.stock);
        order.price = static_cast<int64_t>(msg.price);
        // Genelde trade mesajları yeni emir yaratmaz, sadece execution bilgisi verir
    }

    // ===== SBE fillers =====
    inline void fill_sbe_add(Order &order, const SBEAddOrderMessage &msg) noexcept
    {
        order.price = static_cast<int64_t>(msg.price);
        order.quantity = msg.quantity;
        order.side = (msg.side == 0) ? Side::Buy : Side::Sell;
        order.status = Status::New;

        order.instrument_id = msg.header.schemaId; // veya header.version kullanılabilir
        auto it = instrument_cache_.find(order.instrument_id);
        if (it != instrument_cache_.end())
        {
            order.symbol = it->second.symbol;
        }

        order.timestamp = msg.header.version / 1'000'000'000ULL;
        order.last_update_time = order.timestamp;

        order.message_type = msg.header.templateId;
        order.protocol = Protocol::SBE;

        // order.timestamp_ns = msg.header.version;
    }

    inline void fill_sbe_modify(Order &order, const SBEModifyOrderMessage &msg) noexcept
    {
        order.quantity = msg.newQuantity;
        order.last_update_time = msg.header.version / 1'000'000'000ULL;
    }

    inline void fill_sbe_cancel(Order &order, const SBEDeleteOrderMessage &msg) noexcept
    {
        if (order.filled_quantity > 0)
            order.status = Status::PartiallyFilled_Cancelled;
        else
            order.status = Status::Cancelled;

        order.last_update_time = msg.header.version / 1'000'000'000ULL;
    }

    inline void fill_sbe_trade(Order &order, const SBETradeMessage &msg) noexcept
    {
        order.price = static_cast<int64_t>(msg.price);
        order.filled_quantity += msg.quantity;
        order.instrument_id = msg.header.schemaId;

        if (order.filled_quantity < order.quantity)
            order.status = Status::Partial;
        else
            order.status = Status::Filled;

        auto it = instrument_cache_.find(order.instrument_id);
        if (it != instrument_cache_.end())
        {
            order.symbol = it->second.symbol;
        }

        order.last_update_time = msg.header.version / 1'000'000'000ULL;
        order.message_type = msg.header.templateId;
        order.protocol = Protocol::SBE;
    }
};
