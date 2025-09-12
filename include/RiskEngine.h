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

struct AccountState
{
    __int128 current_exposure; // aktif kaldıraç
    int64_t balance;           // güncel bakiye
    int64_t used_margin;       // açık pozisyonlardan kullanılan margin
    double current_leverage;   // aktif kaldıraç
    int64_t daily_unrealized_pnl;
    int64_t daily_realized_pnl;
    AccountStatus status;
};

// ==========================
// Symbol risk (limits + hot state)
// ==========================
struct alignas(64) SymbolRisk
{
    // ----- Hot state (per-symbol) -----
    int64_t net_position_scaled;     // pozisyon
    int64_t cost_basis_scaled;       // maliyet (pozisyon * entry_price)
    int64_t unrealized_pnl;          // PnL
    int64_t pending_notional_scaled; // bekleyen notional
    int32_t open_orders_count;
    uint8_t pad1[4]; // alignment

    // ----- Market state (son fiyatlar) -----
    int64_t best_bid; // en iyi bid
    int64_t best_ask; // en iyi ask
    int64_t max_price_deviation;

    // ----- Limits (read-only) -----
    int64_t max_position_scaled; // done
    int64_t tick_size_scaled;    // done
    int64_t max_notional_scaled; // done
    uint32_t scale_factor;
    uint32_t min_qty;       // done
    uint32_t max_order_qty; // done

    uint8_t pad2[28]; // alignment
};

// ==========================
// Order risk entry (hot per-active-order)
// ==========================
struct alignas(64) OrderRisk
{
    uint64_t client_order_id;   // internal order ID  //done
    uint64_t exchange_order_id; // exchange order ID (0 if not assigned)
    int64_t price_scaled;       // fixed-point price
    uint32_t symbol_index;      // symbol index
    int32_t remaining_qty;      // remaining qty
    int32_t filled_qty;         // filled qty
    Side side;                  // 0 buy, 1 sell
    Status status;              // small status code
    Venue venue;
    uint8_t pad[25]; // alignment padding
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
    // MaxPositionLimitExceededSymbol = 1 << 0, // Belirlen pozisyon limitinin aşılması
    // MaxPositionLimitExceededAccount = 1 << 24,
    // MaxOrderSizeExceeded = 1 << 1,       // Emir büyüklüğü lot bazında limitin aşılması
    // MinOrderSizeExceeded = 1 << 13,      // Emir büyüklüğü lot bazında limitin aşılması
    // NotionalOrderValueExceeded = 1 << 2, // Emir parasal değeri limitin aşılması
    // MaxNotionalLimitExceeded = 1 << 26,
    // MaxOpenOrdersReachedAccount = 1 << 3, // Toplam açık emir sayısı limit aşımı
    // MaxOpenOrdersReachedSymbol = 1 << 23,

    // Price validity
    // PriceTickInvalid = 1 << 4,       // Fiyat tick size kuralına uymuyor
    // InvalidPriceRange = 1 << 5,      // Emir fiyatı market fiyatına göre geçersiz (çok uzak)
    // FatFingerCheckFailed = 1 << 6,   // Olağan dışı büyük emir boyutu/fiyat sapması
    // PriceOutsideMarketBand = 1 << 7, // Borsa fiyat bandının dışında

    // Order rate & frequency
    MaxCancelRateExceeded = 1 << 8,             // Fazla iptal oranı limiti aşıldı
    MaxOrderRatePerInstrumentExceeded = 1 << 9, // Tek enstrümanda saniye başına emir limiti aşıldı
    // OrderThrottleLimitReached = 1 << 10,        // Kısa sürede çok fazla emir gönderimi

    // Self-trade & duplication
    // SelfTradeDetected = 1 << 11, // Aynı hesapta karşılıklı emir
    // DuplicateOrderID = 1 << 12,  // Aynı ID daha önce kullanılmış

    // Account & instrument
    // RestrictedAccountStatus = 1 << 14,    // Hesap, kısıtlı/askıya alınmış durumda
    // AccountBalanceInsufficient = 1 << 15, // Yetersiz bakiye
    // MaxLeverageExceeded = 1 << 16,        // Kaldıraç limiti aşımı
    // FreeMarginInsufficient = 1 << 25,

    // Loss limits
    // RealizedLossLimitExceeded = 1 << 17,         // Günlük/haftalık max kayıp aşıldı
    // UnrealizedLossLimitExceeded = 1 << 18, // Gerçekleşmemiş zarar limiti

    // Market state
    // VenueTradingHalted = 1 << 19,      // Venue/sembol işlem durduruldu (halt)
    // CircuitBreakerTriggered = 1 << 20, // İç devre kesici tetiklendi

    // Technical
    // MarketDataUnavailable = 1 << 21,  // Market verisi yok (price check yapılamıyor)
    // RiskEngineInternalError = 1 << 22 // Beklenmeyen risk engine hatası

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
    std::array<std::vector<SymbolRisk>, VENUE_COUNT> initialize_symbolrisks() noexcept;
};

// 1) Duplicate Order ID check (cheap)
inline uint32_t check_duplicate_order_id(const RecentIdRing &ring, uint64_t client_order_id) noexcept
{
    // RecentIdRing::SIZE is small and cache-resident (1024). Single-thread writer; many readers ok.
    // Linear scan of 1024 is bounded and deterministic — acceptable for HFT duplicate detection.
    for (size_t i = 0; i < RecentIdRing::SIZE; ++i)
    {
        if (ring.ids[i] == client_order_id)
        {
            return static_cast<uint32_t>(RiskRejectReason::DuplicateOrderID);
        }
    }
    return 0;
}

// 3) Quantity bounds: min_qty/max_order_qty (per-symbol)
inline uint32_t check_quantity_bounds(uint32_t qty, uint32_t min_qty, uint32_t max_order_qty) noexcept
{
    if (UNLIKELY(qty < min_qty))
        return static_cast<uint32_t>(RiskRejectReason::MinOrderSizeExceeded); // reuse code for small orders
    if (UNLIKELY(qty > max_order_qty))
        return static_cast<uint32_t>(RiskRejectReason::MaxOrderSizeExceeded);
    return 0;
}

// 4) Price tick validity (integer scaled arithmetic)
// price_scaled = integer price in "ticks * tick_size_scaled" or something similar.
// We check (price_scaled % tick_size_scaled == 0)
inline uint32_t check_price_tick_valid(int64_t price_scaled, int64_t tick_size_scaled) noexcept
{
    if (UNLIKELY(tick_size_scaled <= 0))
        return static_cast<uint32_t>(RiskRejectReason::RiskEngineInternalError);
    if (UNLIKELY((price_scaled % tick_size_scaled) != 0))
        return static_cast<uint32_t>(RiskRejectReason::PriceTickInvalid);

    return 0;
}

inline uint32_t check_max_open_orders_account(uint32_t current_open_orders,
                                              uint32_t max_open_orders) noexcept
{
    if (UNLIKELY(current_open_orders >= max_open_orders))
        return static_cast<uint32_t>(RiskRejectReason::MaxOpenOrdersReachedAccount);
    return 0;
}

inline uint32_t check_max_open_orders_symbol(uint32_t symbol_open_orders,
                                             uint32_t max_symbol_open_orders) noexcept
{
    if (UNLIKELY(symbol_open_orders >= max_symbol_open_orders))
        return static_cast<uint32_t>(RiskRejectReason::MaxOpenOrdersReachedSymbol);
    return 0;
}

// 7) Self-trade detection (requires minimal per-account orderbook state).
// Here we provide a simple conservative check: if there's an opposite-side open order with same price on same account.
// For true HFT, you'd maintain per-account price->qty map; here we assume a callable check provided externally.
using HasOppositeAtPriceFn = bool (*)(uint32_t account_id, uint32_t symbol_index, int64_t price_scaled, Side side) noexcept;
inline uint32_t check_self_trade(uint32_t account_id, uint32_t symbol_index, int64_t price_scaled, Side side, HasOppositeAtPriceFn hasOpposite) noexcept
{
    if (UNLIKELY(hasOpposite(account_id, symbol_index, price_scaled, side)))
    {
        return static_cast<uint32_t>(RiskRejectReason::SelfTradeDetected);
    }
    return 0;
}

inline uint32_t check_self_trade_order(const Order &order, const auto &orderrisks_map) noexcept
{
    // OrderRisk map’inde aynı account ve symbol için tüm açık emirleri tara
    for (const auto &[id, o] : orderrisks_map)
    {
        if (o.status == Status::Filled || o.status == Status::Cancelled || o.status == Status::Rejected)
            continue;

        if (o.side != order.side && o.symbol_index == order.symbol_index)
        {
            // Aynı price seviyesinde karşı taraf emri var mı?
            if (o.price_scaled == order.price)
                return static_cast<uint32_t>(RiskRejectReason::SelfTradeDetected);
        }
    }
    return 0;
}

// 8) Notional (price * qty) check against max_notional_scaled
inline uint32_t check_notional_value(int64_t price_scaled, uint32_t qty, int64_t max_notional_scaled) noexcept
{
    __int128 notional = static_cast<__int128>(price_scaled) * qty;
    if (UNLIKELY(notional > max_notional_scaled))
        return static_cast<uint32_t>(RiskRejectReason::NotionalOrderValueExceeded);
    return 0;
}

// 9) Net position / max position check (per-symbol, scaled)
inline uint32_t check_max_position_limit_symbol(int64_t net_position_scaled, int64_t add_qty_signed_scaled, int64_t max_position_scaled) noexcept
{
    int64_t new_pos = net_position_scaled + add_qty_signed_scaled;
    if (UNLIKELY(std::llabs(new_pos) > std::llabs(max_position_scaled)))
        return static_cast<uint32_t>(RiskRejectReason::MaxPositionLimitExceededSymbol);

    return 0;
}

inline uint32_t check_max_position_limit_account(int64_t net_position_account_scaled, int64_t add_qty_signed_scaled, int64_t max_position_account_scaled) noexcept
{
    int64_t new_pos_account = net_position_account_scaled + add_qty_signed_scaled;
    if (UNLIKELY(std::llabs(new_pos_account) > std::llabs(max_position_account_scaled)))
        return static_cast<uint32_t>(RiskRejectReason::MaxPositionLimitExceededAccount);

    return 0;
}

// 10) Account-level balance & leverage checks (requires external state)
// We'll accept current margin usage and account_balance as inputs.
inline uint32_t check_account_risk(AccountState &acc_stat, AccountLimits &acc_lim, const int64_t price_scaled, const uint32_t qty) noexcept
{
    // Account inactive / suspended
    if (UNLIKELY(acc_stat.status != static_cast<uint8_t>(AccountStatus::Active)))
    {
        return static_cast<uint32_t>(RiskRejectReason::RestrictedAccountStatus);
    }

    __int128 estimated_notional = static_cast<__int128>(price_scaled) * qty;
    __int128 estimated_margin_needed = estimated_notional / acc_stat.current_leverage;

    // Equity = balance + unrealized_pnl
    const double equity = acc_stat.balance + acc_stat.daily_unrealized_pnl;

    // Free margin check
    const double free_margin = equity - acc_stat.used_margin;
    if (UNLIKELY(free_margin < estimated_margin_needed))
    {
        return static_cast<uint32_t>(RiskRejectReason::FreeMarginInsufficient);
    }

    // Balance check (extra guard)
    if (UNLIKELY(acc_stat.balance < estimated_margin_needed))
    {
        return static_cast<uint32_t>(RiskRejectReason::AccountBalanceInsufficient);
    }

    // Leverage check
    if (UNLIKELY(acc_stat.current_leverage > acc_lim.max_leverage))
    {
        return static_cast<uint32_t>(RiskRejectReason::MaxLeverageExceeded);
    }

    // Exposure / notional check
    if (UNLIKELY((acc_stat.current_exposure + estimated_notional) > acc_lim.max_notional))
    {
        return static_cast<uint32_t>(RiskRejectReason::MaxNotionalLimitExceeded);
    }

    // Daily loss check
    check_max_drawdown(acc_stat.daily_realized_pnl, acc_lim.max_daily_loss);
    return 0; // OK
}

// 11) Max daily loss / drawdown (account-level)
inline uint32_t check_max_drawdown(int64_t daily_realized_pnl, int64_t max_daily_loss) noexcept
{
    if (UNLIKELY(daily_realized_pnl <= -max_daily_loss))
    {
        return static_cast<uint32_t>(RiskRejectReason::RealizedLossLimitExceeded);
    }
    return 0;
}

// 12) Unrealized loss / open PnL limit (account-level)
inline uint32_t check_unrealized_loss_limit(int64_t daily_unrealized_pnl,
                                            int64_t max_unrealized_loss) noexcept
{
    // Unrealized zarar limitini kontrol et
    if (UNLIKELY(daily_unrealized_pnl <= -max_unrealized_loss))
    {
        return static_cast<uint32_t>(RiskRejectReason::UnrealizedLossLimitExceeded);
    }
    return 0; // OK
}

// Market data availability + price band check helper
inline uint32_t check_market_data_and_price_band_helper(
    int64_t best_bid_scaled,
    int64_t best_ask_scaled,
    int64_t price_scaled,
    int64_t max_price_deviation_scaled) noexcept
{
    // Market data yoksa
    if (UNLIKELY(best_bid_scaled == 0 && best_ask_scaled == 0))
        return static_cast<uint32_t>(RiskRejectReason::MarketDataUnavailable);

    // Mid-price
    int64_t mid_price = (best_bid_scaled + best_ask_scaled) / 2;

    // Price deviation kontrolü
    if (UNLIKELY(max_price_deviation_scaled > 0 &&
                 std::llabs(price_scaled - mid_price) > max_price_deviation_scaled))
    {
        return static_cast<uint32_t>(RiskRejectReason::PriceOutsideMarketBand);
    }

    return 0; // OK
}

// 13) Venue trading halted / circuit breaker
inline uint32_t check_venue_halt_and_circuit(bool venue_halted, bool circuit_breaker) noexcept
{
    if (UNLIKELY(venue_halted))
        return static_cast<uint32_t>(RiskRejectReason::VenueTradingHalted);
    if (UNLIKELY(circuit_breaker))
        return static_cast<uint32_t>(RiskRejectReason::CircuitBreakerTriggered);
    return 0;
}

// 14) Fat-finger check (very large quantity or price far from market)
// This should be last among cheap checks but before expensive ones.
inline uint32_t check_fat_finger_int(
    uint32_t qty,
    int64_t price_scaled,
    int64_t best_bid_scaled,
    int64_t best_ask_scaled,
    uint32_t fat_finger_qty_threshold,
    int64_t fat_finger_ratio_scaled // scaled ratio yoksa direkt double gibi geçebiliriz
    ) noexcept
{
    if (UNLIKELY(qty >= fat_finger_qty_threshold))
        return static_cast<uint32_t>(RiskRejectReason::FatFingerCheckFailed);

    int64_t mid_price = (best_bid_scaled + best_ask_scaled) / 2;

    if (UNLIKELY(mid_price > 0))
    {
        // integer division ile ratio kontrolü
        int64_t ratio_check = price_scaled / mid_price;
        if (UNLIKELY(ratio_check > fat_finger_ratio_scaled))
            return static_cast<uint32_t>(RiskRejectReason::InvalidPriceRange);
    }

    return 0;
}

// ---------------------------
// Combined top-level check that calls the above in prioritized order
// ---------------------------
/*
 Parameters explained:
  - order: pointer to Order (hot struct)
  - ordRisk: pointer to OrderRisk (per-active-order hot)
  - sym: per-symbol SymbolRisk (read mostly)
  - acctLimits: AccountLimits for account
  - forbidden_table: bool array indexed by symbol_index
  - forbidden_table_size: size of the table
  - recent_ring: pointer to recent id ring for duplication detection
  - rate_bucket: per-account or per-symbol RateLimitBucket
  - hasOppositeFn: function pointer to check self-trade (concrete implementation provided externally)
  - pricing helpers: double price_double (converted), now_ns, etc.
  - Outputs: returns a 32-bit bitmask of reject reasons (0==ok)
*/
inline uint32_t check_all_risks_for_order() noexcept
{

    return 0;
}