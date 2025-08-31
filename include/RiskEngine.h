#pragma once

#include "Order.h"
#include "Protocol-Venue.h"
#include "HashTables.h"
#include "common.h"
#include "MarketBook.h"
#include "GeneratedLimits.h"

#include <cstring>
#include <boost/lockfree/spsc_queue.hpp>
#include <array>
#include <cstdint>
#include <cstddef>
#include <vector>
#include <absl/container/flat_hash_map.h>
#include <x86intrin.h>

// ==========================
// Symbol risk (limits + hot state)
// ==========================
struct alignas(64) SymbolRisk
{
    // ----- Hot state (per-symbol) -----
    int64_t net_position_scaled;     // pozisyon
    int64_t cost_basis_scaled;       // maliyet (pozisyon * entry_price)
    double unrealized_pnl;           // PnL
    int64_t pending_notional_scaled; // bekleyen notional
    int32_t open_orders_count;
    uint8_t pad[4]; // alignment

    // ----- Market state (son fiyatlar) -----
    double best_bid; // en iyi bid
    double best_ask; // en iyi ask

    // ----- Limits (read-only) -----
    double max_daily_loss;
    int64_t tick_size_scaled;
    int64_t max_notional_scaled;
    uint32_t min_qty;
    uint32_t max_order_qty;
};

// ==========================
// Order risk entry (hot per-active-order)
// ==========================
struct alignas(64) OrderRisk
{
    uint64_t client_order_id;   // internal order ID
    uint64_t exchange_order_id; // exchange order ID (0 if not assigned)
    int64_t price_scaled;       // fixed-point price
    uint32_t symbol_index;      // symbol index
    uint32_t account_id;        // account index
    int32_t remaining_qty;      // remaining qty
    int32_t filled_qty;         // filled qty
    Side side;                  // 0 buy, 1 sell
    Status status;              // small status code
    Venue venue;
    uint8_t pad[21]; // alignment padding
};

// ==========================
// Rate limit bucket (per-symbol/account)
// ==========================
struct alignas(64) RateLimitBucket
{
    uint64_t tokens;             // remaining tokens
    uint64_t last_refill_ns;     // last refill timestamp
    uint64_t capacity;           // max tokens
    uint64_t refill_interval_ns; // refill interval in ns
    uint8_t pad[32];             // 64-byte alignment
};

// ==========================
// Recent ID ring (duplicate detection)
// ==========================
struct RecentIdRing
{
    static constexpr size_t SIZE = 1024;
    std::array<uint64_t, SIZE> ids;
    size_t head = 0; // single-thread writer
};

enum class RiskRejectReason : uint32_t
{
    Unknown = 0, // Tanımsız ret sebebi
    // Position & exposure limits
    MaxPositionLimitExceeded = 1 << 0,   // Belirlen pozisyon limitinin aşılması
    MaxOrderSizeExceeded = 1 << 1,       // Emir büyüklüğü lot bazında limitin aşılması
    NotionalOrderValueExceeded = 1 << 2, // Emir parasal değeri limitin aşılması
    MaxOpenOrdersReached = 1 << 3,       // Toplam açık emir sayısı limit aşımı

    // Price validity
    PriceTickInvalid = 1 << 4,       // Fiyat tick size kuralına uymuyor
    InvalidPriceRange = 1 << 5,      // Emir fiyatı market fiyatına göre geçersiz (çok uzak)
    FatFingerCheckFailed = 1 << 6,   // Olağan dışı büyük emir boyutu/fiyat sapması
    PriceOutsideMarketBand = 1 << 7, // Borsa fiyat bandının dışında

    // Order rate & frequency
    MaxCancelRateExceeded = 1 << 8,             // Fazla iptal oranı limiti aşıldı
    MaxOrderRatePerInstrumentExceeded = 1 << 9, // Tek enstrümanda saniye başına emir limiti aşıldı
    OrderThrottleLimitReached = 1 << 10,        // Kısa sürede çok fazla emir gönderimi

    // Self-trade & duplication
    SelfTradeDetected = 1 << 11, // Aynı hesapta karşılıklı emir
    DuplicateOrderID = 1 << 12,  // Aynı ID daha önce kullanılmış

    // Account & instrument
    ForbiddenInstrument = 1 << 13,        // Bu sembol trade edilemez
    RestrictedAccountStatus = 1 << 14,    // Hesap, kısıtlı/askıya alınmış durumda
    AccountBalanceInsufficient = 1 << 15, // Yetersiz bakiye
    MaxLeverageExceeded = 1 << 16,        // Kaldıraç limiti aşımı

    // Loss limits
    MaxDrawdownExceeded = 1 << 17,         // Günlük/haftalık max kayıp aşıldı
    UnrealizedLossLimitExceeded = 1 << 18, // Gerçekleşmemiş zarar limiti

    // Market state
    VenueTradingHalted = 1 << 19,      // Venue/sembol işlem durduruldu (halt)
    CircuitBreakerTriggered = 1 << 20, // İç devre kesici tetiklendi

    // Technical
    MarketDataUnavailable = 1 << 21,  // Market verisi yok (price check yapılamıyor)
    RiskEngineInternalError = 1 << 22 // Beklenmeyen risk engine hatası

};

struct OrderWithRejectReason
{
    Order *order;
    uint32_t RejectReason;
};

inline constexpr size_t ORDER_QUEUE_CAPACITY = 65536;
inline constexpr size_t REJECTORDER_QUEUE_SIZE = 1024;

using spscRejectOrderQueue_t = boost::lockfree::spsc_queue<OrderWithRejectReason, boost::lockfree::capacity<REJECTORDER_QUEUE_SIZE>>;
using spscOrderQueue_t = boost::lockfree::spsc_queue<Order *, boost::lockfree::capacity<ORDER_QUEUE_CAPACITY>>;

class RiskEngine
{
private:
    spscOrderQueue_t &store_to_risk_;
    spscOrderQueue_t &strategy_to_risk_;
    spscRejectOrderQueue_t &risk_to_strategy_;
    spscOrderQueue_t &risk_to_builder_;

    std::array<std::vector<SymbolRisk>, VENUE_COUNT> symbolrisks_;
    std::array<RateLimitBucket, VENUE_COUNT> ratelimits_;

    absl::flat_hash_map<uint64_t, OrderRisk> orderrisks_map_;

    HashTables &hashtables_;
    MarketBook &marketbook_;

public:
    RiskEngine(spscOrderQueue_t &store_to_risk, spscOrderQueue_t &strategy_to_risk, spscRejectOrderQueue_t &risk_to_strategy, spscOrderQueue_t &risk_to_builder, HashTables &hashtables, MarketBook &marketbook) noexcept;

    inline void risk_update() noexcept
    {
        Order *ord;
        while (strategy_to_risk_.pop(ord))
        {
            // 1) Rate limit kontrolü
            update_rate_limit(*ord);

            // 2) Symbol risk güncelle
            update_symbol_risk(*ord);

            if (ord->isOurOrder)
                update_order_risk(*ord);
        }
    }

private:
    inline void update_unrealized_pnl(SymbolRisk &symRisk) noexcept
    {
        if (symRisk.net_position_scaled == 0)
        {
            symRisk.unrealized_pnl = 0.0;
            return;
        }

        const double avg_entry_price = static_cast<double>(symRisk.cost_basis_scaled) /
                                       static_cast<double>(symRisk.net_position_scaled);

        const double position_abs = static_cast<double>(std::abs(symRisk.net_position_scaled));

        if (symRisk.net_position_scaled > 0)
        {
            // Long pozisyon → çıkış bid üzerinden
            symRisk.unrealized_pnl = (symRisk.best_bid - avg_entry_price) * position_abs;
        }
        else
        {
            // Short pozisyon → çıkış ask üzerinden
            symRisk.unrealized_pnl = (avg_entry_price - symRisk.best_ask) * position_abs;
        }
    }

    inline void update_symbol_risk(const Order &order) noexcept
    {
        auto venue_index = static_cast<std::underlying_type_t<Venue>>(order.venue);
        auto &symRisk = symbolrisks_[venue_index][hashtables_.getIndex(venue_index, order.symbol)];

        // ----- Bize ait order ise -----
        if (order.isOurOrder)
        {
            switch (order.status)
            {
            // --- Yeni veya beklemede ---
            case Status::New:
            case Status::PendingNew:
            case Status::Accepted:
            case Status::PendingSubmit:
                symRisk.open_orders_count++;
                symRisk.pending_notional_scaled += order.price * order.quantity;
                break;

            // --- Parsiyel dolum ---
            case Status::Partial:
            {
                int8_t side_mult = (order.side == Side::Buy ? 1 : -1);

                symRisk.net_position_scaled += side_mult * order.filled_quantity;
                symRisk.pending_notional_scaled -= order.filled_quantity * order.price;
                symRisk.cost_basis_scaled += side_mult * (order.price * order.filled_quantity);
                break;
            }
            // --- Tam dolum ---
            case Status::Filled:
            {
                int8_t side_mult = (order.side == Side::Buy ? 1 : -1);

                symRisk.net_position_scaled += side_mult * order.filled_quantity;
                symRisk.pending_notional_scaled -= order.filled_quantity * order.price;
                symRisk.cost_basis_scaled += side_mult * (order.price * order.filled_quantity);
                symRisk.open_orders_count--;
                break;
            }
            case Status::Cancelled:
            case Status::Expired:
            case Status::DoneForDay:
            case Status::Stopped:
            case Status::Suspended:
            case Status::PartiallyFilled_Cancelled:
            case Status::Rejected:
            case Status::CancelReject:
            case Status::ReplaceReject:
                symRisk.open_orders_count--;
                symRisk.pending_notional_scaled = 0;
                break;

            case Status::PendingReplace:
            case Status::Restated:
            case Status::Unknown:
            default:
                break;
            }
        }
        // ----- Başkasına ait market verisi -----
        else
        {
            symRisk.best_bid = marketbook_.best_bid(marketbook_.get_symBook(order));
            symRisk.best_ask = marketbook_.best_ask(marketbook_.get_symBook(order));
            update_unrealized_pnl(symRisk);
        }

        // Güvenlik: negatif değerleri engelle
        if (UNLIKELY(symRisk.pending_notional_scaled < 0))
            symRisk.pending_notional_scaled = 0;
        if (UNLIKELY(symRisk.open_orders_count < 0))
            symRisk.open_orders_count = 0;
    }

    inline void update_order_risk(const Order &order) noexcept
    {
        auto [it, inserted] = orderrisks_map_.try_emplace(
            order.client_order_id,
            OrderRisk{});

        OrderRisk &o = it->second;

        o.client_order_id = order.client_order_id;
        o.exchange_order_id = order.order_id;
        o.price_scaled = order.price;
        o.remaining_qty = std::max<int32_t>(order.quantity - order.filled_quantity, 0);
        o.filled_qty = order.filled_quantity;
        o.status = order.status;
        o.side = order.side;
        o.venue = order.venue;

        if (inserted)
            o.symbol_index = hashtables_.getIndex(static_cast<std::underlying_type_t<Venue>>(order.venue), order.symbol);
    }

    inline void update_rate_limit(const Order &order) noexcept
    {
        static constexpr double ns_per_cycle = 1 / 3e9; // 1 cycle ≈ 0.333 ns
        uint64_t now_ns = static_cast<uint64_t>(__rdtsc() * ns_per_cycle);

        auto venue_index = static_cast<std::underlying_type_t<Venue>>(order.venue);
        auto &ratelimit = ratelimits_[venue_index];

        if (now_ns - ratelimit.last_refill_ns > ratelimit.refill_interval_ns)
        {
            ratelimit.tokens = ratelimit.capacity;
            ratelimit.last_refill_ns = now_ns;
        }

        if (ratelimit.tokens > 0)
        {
            ratelimit.tokens--;
        }
        else
        {
            // rate limit violation → reject
            OrderWithRejectReason rej{const_cast<Order *>(&order),
                                      static_cast<uint32_t>(RiskRejectReason::OrderThrottleLimitReached)};
            risk_to_strategy_.push(rej);
        }
    }

    std::array<RateLimitBucket, VENUE_COUNT> initialize_ratelimits() noexcept;
};
