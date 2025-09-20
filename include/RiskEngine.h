#pragma once

#include "Order.h"
#include "Protocol-Venue.h"
#include "HashTables.h"
#include "common.h"
#include "MarketBook.h"
#include "Limits.h"

#include <cstring>
#include <boost/lockfree/spsc_queue.hpp>
#include <array>
#include <cstdint>
#include <cstddef>
#include <vector>
#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>
#include <x86intrin.h>

// ==========================
// Rate limit bucket (per-symbol/account)
// ==========================
struct OrderRateLimit
{
    uint64_t tokens;             // remaining tokens
    uint64_t last_refill_ns;     // last refill timestamp
    uint64_t capacity;           // max tokens
    uint64_t refill_interval_ns; // refill interval in ns
};

struct CancelRateLimit
{
    uint64_t last_cancel_timestamp_ns; 
    uint32_t cancel_count_window;
    uint32_t max_cancels_per_sec;     
};

enum AccountStatus : uint8_t
{
    Inactive = 0,
    Active = 1,
    Suspended = 2,
    Closed = 3
};

struct alignas(64) AccountRisk
{
    int64_t current_exposure = 0; 
    int64_t balance = 0;           // güncel bakiye
    int64_t used_margin = 0;       // açık pozisyonlardan kullanılan margin
    int64_t current_leverage = 0;   // aktif kaldıraç
    int64_t daily_realized_pnl = 0;
    int64_t total_unrealized_pnl = 0;
    AccountStatus status = AccountStatus::Inactive;
    uint8_t pad1[7];
    
    OrderRateLimit orderratelimit; // rate limit bucket
    CancelRateLimit cancelratelimit; // rate limit bucket
    uint8_t pad2[16];
};

// ==========================
// Symbol risk (limits + hot state)
// ==========================
struct alignas(64) SymbolRisk
{
    // ----- Hot state (per-symbol) -----
    int64_t net_position_scaled = 0;     // pozisyon
    int64_t cost_basis_scaled = 0;       // maliyet (pozisyon * entry_price)
    int64_t unrealized_pnl = 0;          // PnL
    int64_t pending_notional_scaled = 0; // bekleyen notional
    int64_t best_bid = 0; // en iyi bid
    int64_t best_ask = 0; // en iyi ask
    uint32_t open_orders_count = 0;
    uint8_t pad1[12]; // alignment padding

    OrderRateLimit orderratelimit; // rate limit bucket
    CancelRateLimit cancelratelimit; // rate limit bucket
    uint8_t pad2[16]; // alignment padding
    
};

// ==========================
// Order risk entry (hot per-active-order)
// ==========================
struct alignas(64) OrderRisk
{
    int64_t price_scaled;       // fixed-point price
    uint32_t symbol_index;      // symbol index
    uint32_t remaining_qty;     // kalan miktar
    Side side;                  // 0 buy, 1 sell
    Status status;              // order durumu
    uint8_t pad[25]; // alignment padding
};

enum class RiskRejectReason : uint32_t
{
    Unknown = 0, // Tanımsız ret sebebi
    // Position & exposure limits
    MaxPositionLimitExceededSymbol = 1 << 0, // Belirlen pozisyon limitinin aşılması
    MaxPositionLimitExceededAccount = 1 << 24,
    MaxOrderSizeExceeded = 1 << 1,       // Emir büyüklüğü lot bazında limitin aşılması
    MinOrderSizeExceeded = 1 << 13,      // Emir büyüklüğü lot bazında limitin aşılması
    NotionalOrderValueExceeded = 1 << 2, // Emir parasal değeri limitin aşılması
    MaxNotionalLimitExceeded = 1 << 26,
    MaxOpenOrdersReachedAccount = 1 << 3, // Toplam açık emir sayısı limit aşımı
    MaxOpenOrdersReachedSymbol = 1 << 23,

    // Price validity
    PriceTickInvalid = 1 << 4,       // Fiyat tick size kuralına uymuyor
    InvalidPriceRange = 1 << 5,      // Emir fiyatı market fiyatına göre geçersiz (çok uzak)
    FatFingerCheckFailed = 1 << 6,   // Olağan dışı büyük emir boyutu/fiyat sapması
    PriceOutsideMarketBand = 1 << 7, // Borsa fiyat bandının dışında

    // Order rate & frequency
    MaxOrderRateLimitExceededAccount = 1 << 8,             
    MaxCancelRateLimitExceededAccount = 1 << 9,
    MaxOrderRateLimitExceededSymbol = 1 << 10,    
    MaxCancelRateLimitExceededSymbol = 1 << 27,     
   
    // Self-trade & duplication
    SelfTradeDetected = 1 << 11, // Aynı hesapta karşılıklı emir
    DuplicateOrderID = 1 << 12,  // Aynı ID daha önce kullanılmış

    // Account & instrument
    RestrictedAccountStatus = 1 << 14,    // Hesap, kısıtlı/askıya alınmış durumda
    AccountBalanceInsufficient = 1 << 15, // Yetersiz bakiye
    MaxLeverageExceeded = 1 << 16,        // Kaldıraç limiti aşımı
    FreeMarginInsufficient = 1 << 25,

    // Loss limits
    RealizedLossLimitExceeded = 1 << 17,         // Günlük/haftalık max kayıp aşıldı
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
    Order &order;
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

    std::array<AccountRisk, VENUE_COUNT> accountrisks_;
    std::array<std::vector<SymbolRisk>, VENUE_COUNT> symbolrisks_;
    std::array<absl::flat_hash_map<uint64_t, OrderRisk>, VENUE_COUNT> orderrisks_;

    std::array<absl::flat_hash_set<uint64_t>, VENUE_COUNT> order_ids; 

    HashTables &hashtables_;
    MarketBook &marketbook_;
    Limits &limits_;

public:
    RiskEngine(spscOrderQueue_t &store_to_risk, spscOrderQueue_t &strategy_to_risk, spscRejectOrderQueue_t &risk_to_strategy, spscOrderQueue_t &risk_to_builder, HashTables &hashtables, MarketBook &marketbook, Limits &limits) noexcept;

    inline void update_risk() noexcept
    {
        Order *order;
        while (store_to_risk_.pop(order))
        {
            const uint8_t venue_index = static_cast<uint8_t>(order->venue);

            update_symbol_risk(*order, venue_index);

            if (order->isOurOrder) {
                update_order_risk(*order, venue_index);
                update_account_risk(*order, venue_index);
            }
        }    
    }

    inline uint32_t check_risk() noexcept
        {

            return 0;
        }

private:
    inline void update_unrealized_pnl(SymbolRisk &symRisk) noexcept
    {
        if (symRisk.net_position_scaled == 0)
        {
            symRisk.unrealized_pnl = 0.0;
            return;
        }

        const int64_t avg_entry_price = symRisk.cost_basis_scaled / symRisk.net_position_scaled;

        const int64_t position_abs = std::abs(symRisk.net_position_scaled);

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

    inline void update_symbol_risk(const Order &order, const uint8_t venue_index) noexcept
    {
        auto &accRisk = accountrisks_[venue_index];
        auto &symRisk = symbolrisks_[venue_index][hashtables_.getIndex(venue_index, order.symbol)];

        int64_t filled_qty_int64 = static_cast<int64_t>(order.filled_quantity);

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
                symRisk.pending_notional_scaled += order.price * static_cast<int64_t>(order.quantity);
                break;

            // --- Parsiyel dolum ---
            case Status::Partial:
            {
                int8_t side_mult = (order.side == Side::Buy ? 1 : -1);

                symRisk.net_position_scaled += side_mult * filled_qty_int64;
                symRisk.pending_notional_scaled -= filled_qty_int64 * order.price;
                symRisk.cost_basis_scaled += side_mult * (order.price * filled_qty_int64);
                break;
            }
            // --- Tam dolum ---
            case Status::Filled:
            {
                int8_t side_mult = (order.side == Side::Buy ? 1 : -1);

                symRisk.net_position_scaled += side_mult * filled_qty_int64;
                symRisk.pending_notional_scaled -= filled_qty_int64 * order.price;
                symRisk.cost_basis_scaled += side_mult * (order.price * filled_qty_int64);
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
            accRisk.total_unrealized_pnl -= symRisk.unrealized_pnl;
            update_unrealized_pnl(symRisk);
            accRisk.total_unrealized_pnl += symRisk.unrealized_pnl;
        }

        // Güvenlik: negatif değerleri engelle
        if (UNLIKELY(symRisk.pending_notional_scaled < 0))
            symRisk.pending_notional_scaled = 0;
        if (UNLIKELY(symRisk.open_orders_count < 0))
            symRisk.open_orders_count = 0;
    }

    inline void update_order_risk(const Order &order, const uint8_t venue_index) noexcept
    {
        auto [it, inserted] = orderrisks_[venue_index].try_emplace(
            order.price,
            OrderRisk{});

        OrderRisk &o = it->second;

        o.price_scaled = order.price;
        o.remaining_qty = std::max<uint32_t>(order.quantity - order.filled_quantity, 0);

        constexpr uint64_t terminal_mask =
            (1ULL << static_cast<uint64_t>(Status::Cancelled))
            | (1ULL << static_cast<uint64_t>(Status::Expired))
            | (1ULL << static_cast<uint64_t>(Status::DoneForDay))
            | (1ULL << static_cast<uint64_t>(Status::Stopped))
            | (1ULL << static_cast<uint64_t>(Status::Rejected))
            | (1ULL << static_cast<uint64_t>(Status::CancelReject))
            | (1ULL << static_cast<uint64_t>(Status::ReplaceReject))
            | (1ULL << static_cast<uint64_t>(Status::PartiallyFilled_Cancelled));

        if (o.remaining_qty == 0 || (terminal_mask & (1 << static_cast<uint64_t>(order.status)))) {        
            orderrisks_[venue_index].erase(it);
            return;
        }

        o.status = order.status;
        o.side = order.side;
    
        if (inserted)
            o.symbol_index = hashtables_.getIndex(venue_index, order.symbol);
    }

    inline void update_account_risk(const Order &order, const uint8_t venue_index) noexcept
    {
        auto &accRisk = accountrisks_[venue_index];
        const int8_t side_mult = (order.side == Side::Buy ? 1 : -1);
        const int64_t nominal = order.price * static_cast<int64_t>(order.last_exec_quantity);

        switch (order.status) {

            // --- Emir gönderildi / kabul edildi (blokaj başlar) ---
            case Status::New:
            case Status::PendingNew:
            case Status::Accepted:
            case Status::PendingSubmit: {
                accRisk.current_exposure += nominal * side_mult;
                accRisk.used_margin += nominal;
                break;
            }

            // --- Parsiyel dolum ---
            case Status::Partial: {
                accRisk.balance -= nominal; //+ fee;
                break;
            }

            // --- Tam dolum ---
            case Status::Filled: {
                accRisk.balance -= nominal; //+ fee;
                accRisk.used_margin -= nominal;
                break;
            }

            // --- İptal veya reddedilme (blokaj geri çözülür) ---
            case Status::Cancelled:
            case Status::Expired:
            case Status::DoneForDay:
            case Status::Stopped:
            case Status::PartiallyFilled_Cancelled:
            case Status::Rejected:
            case Status::CancelReject:
            case Status::ReplaceReject: {
                accRisk.current_exposure -= nominal * side_mult;
                accRisk.used_margin -= nominal;
                break;
            }

            // --- Değişiklik veya belirsiz durum ---
            case Status::PendingReplace:
            case Status::Restated:
            case Status::Unknown:
            default:
                break;
        }

        if (accRisk.balance != 0) 
            accRisk.current_leverage = accRisk.current_exposure / accRisk.balance;
        
    }


    inline void check_order_rate_limit(Order &order, const uint8_t venue_index) noexcept
    {
        static constexpr double ns_per_cycle = 1 / 3e9; // 1 cycle ≈ 0.333 ns
        uint64_t now_ns = static_cast<uint64_t>(__rdtsc() * ns_per_cycle);

        auto &accRisk = accountrisks_[venue_index];
        auto &orderratelimit = accRisk.orderratelimit;

        if (now_ns - orderratelimit.last_refill_ns > orderratelimit.refill_interval_ns)
        {
            orderratelimit.tokens = orderratelimit.capacity;
            orderratelimit.last_refill_ns = now_ns;
        }

        if (orderratelimit.tokens > 0)
        {
            orderratelimit.tokens--;
        }
        else
        {
            OrderWithRejectReason rej{order, static_cast<uint32_t>(RiskRejectReason::MaxOrderRateLimitExceededAccount)};
            risk_to_strategy_.push(rej);
        }

        auto &symRisk = symbolrisks_[venue_index][hashtables_.getIndex(venue_index, order.symbol)];
        auto &sym_orderratelimit = symRisk.orderratelimit;

        if (now_ns - sym_orderratelimit.last_refill_ns > sym_orderratelimit.refill_interval_ns)
        {
            sym_orderratelimit.tokens = sym_orderratelimit.capacity;
            sym_orderratelimit.last_refill_ns = now_ns;
        }

        if (sym_orderratelimit.tokens > 0)
        {
            sym_orderratelimit.tokens--;
        }
        else
        {
            OrderWithRejectReason rej{order, static_cast<uint32_t>(RiskRejectReason::MaxOrderRateLimitExceededSymbol)};
            risk_to_strategy_.push(rej);
        }
        
    }

    inline void check_cancel_rate_limit(Order &order, const uint8_t venue_index) noexcept
    {
        static constexpr double ns_per_cycle = 1 / 3e9; // 1 cycle ≈ 0.333 ns
        uint64_t now_ns = static_cast<uint64_t>(__rdtsc() * ns_per_cycle);

        auto &accRisk = accountrisks_[venue_index];
        auto &cancelratelimit = accRisk.cancelratelimit;

        if (now_ns - cancelratelimit.last_cancel_timestamp_ns > 1'000'000'000ULL)
        {
            cancelratelimit.cancel_count_window = 0;
            cancelratelimit.last_cancel_timestamp_ns = now_ns;
        }
        
        cancelratelimit.cancel_count_window++;
        
        if (cancelratelimit.cancel_count_window > cancelratelimit.max_cancels_per_sec)
        {
            OrderWithRejectReason rej{order, static_cast<uint32_t>(RiskRejectReason::MaxCancelRateLimitExceededAccount)};
            risk_to_strategy_.push(rej);
        }

        auto &symRisk = symbolrisks_[venue_index][hashtables_.getIndex(venue_index, order.symbol)];
        auto &sym_cancelratelimit = symRisk.cancelratelimit;
        
        if (now_ns - sym_cancelratelimit.last_cancel_timestamp_ns > 1'000'000'000ULL)
        {
            sym_cancelratelimit.cancel_count_window = 0;
            sym_cancelratelimit.last_cancel_timestamp_ns = now_ns;
        }

        sym_cancelratelimit.cancel_count_window++;

        if (sym_cancelratelimit.cancel_count_window > sym_cancelratelimit.max_cancels_per_sec)
        {
            OrderWithRejectReason rej{order, static_cast<uint32_t>(RiskRejectReason::MaxCancelRateLimitExceededSymbol)};
            risk_to_strategy_.push(rej);
        }
    }

    std::array<AccountRisk, VENUE_COUNT> initialize_accountrisks() noexcept;
    std::array<std::vector<SymbolRisk>, VENUE_COUNT> initialize_symbolrisks() noexcept;


    // 1) Duplicate Order ID check (cheap)
    inline uint32_t check_duplicate_order_id(const auto &order_ids, const Order &order, const uint8_t venue_index) noexcept
    {
        auto it = order_ids[venue_index].find(order.client_order_id);

            if (it != order_ids[venue_index].end())
                return static_cast<uint32_t>(RiskRejectReason::DuplicateOrderID);

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

    // 7) Self-trade detection 
    inline uint32_t check_self_trade_order(const Order &order, const auto &orderrisks_, const uint8_t venue_index) noexcept
    {
        auto it = orderrisks_[venue_index].find(order.price);    

        if(it != orderrisks_[venue_index].end())
        {
            if (it->second.side != order.side && it->second.symbol_index == hashtables_.getIndex(venue_index, order.symbol))
                    return static_cast<uint32_t>(RiskRejectReason::SelfTradeDetected);
        }
        return 0;
    }

    // 8) Notional (price * qty) check against max_notional_scaled
    inline uint32_t check_notional_value(int64_t price_scaled, uint32_t qty, int64_t max_notional_scaled) noexcept
    {
        int64_t notional = price_scaled * qty;
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


    inline uint32_t check_acc_status(AccountStatus status) noexcept
    {
        // Account inactive / suspended
        if (UNLIKELY(status != static_cast<uint8_t>(AccountStatus::Active)))
            return static_cast<uint32_t>(RiskRejectReason::RestrictedAccountStatus);

        return 0;
    }

    inline uint32_t check_free_margin(const int64_t balance, const int64_t total_unrealized_pnl, const __int128 &estimated_margin_needed, const int64_t used_margin) noexcept
    {
        // Free margin check
        const int64_t equity = balance + total_unrealized_pnl;
        const int64_t free_margin = equity - used_margin;

        if (UNLIKELY(free_margin < estimated_margin_needed))
            return static_cast<uint32_t>(RiskRejectReason::FreeMarginInsufficient);
        
        return 0;
    }

    inline uint32_t check_balance(const int64_t balance, const int64_t &estimated_margin_needed) noexcept
    {
        if (UNLIKELY(balance < estimated_margin_needed))
            return static_cast<uint32_t>(RiskRejectReason::AccountBalanceInsufficient);
        
        return 0;
    }

    inline uint32_t check_max_leverage(const double current_leverage, const int64_t max_leverage) noexcept
    {
        if (UNLIKELY(current_leverage > max_leverage))
            return static_cast<uint32_t>(RiskRejectReason::MaxLeverageExceeded);
        
        return 0;
    }

    inline uint32_t check_max_notional(const int64_t current_exposure, const int64_t &estimated_notional, const int64_t max_notional) noexcept
    {
        if (UNLIKELY((current_exposure + estimated_notional) > max_notional))
            return static_cast<uint32_t>(RiskRejectReason::MaxNotionalLimitExceeded);
        return 0;
        
    }

    inline uint32_t check_max_drawdown(const int64_t daily_realized_pnl, const int64_t max_daily_loss) noexcept
    {
        if (UNLIKELY(daily_realized_pnl <= -max_daily_loss))
            return static_cast<uint32_t>(RiskRejectReason::RealizedLossLimitExceeded);
        
        return 0;
    }

    // 10) Account-level balance & leverage checks (requires external state)
    // We'll accept current margin usage and account_balance as inputs.
    inline uint32_t check_account_risk(AccountRisk &accRisk, AccountLimit &accLim, const int64_t price_scaled, const uint32_t qty) noexcept
    {
        check_acc_status(accRisk.status);

        int64_t estimated_notional = price_scaled * qty;
        int64_t estimated_margin_needed = estimated_notional / accRisk.current_leverage * 1'000'000ULL; // assuming leverage is scaled by 1,000,000    

        check_free_margin(accRisk.balance, accRisk.total_unrealized_pnl, estimated_margin_needed, accRisk.used_margin);   
        check_balance(accRisk.balance, estimated_margin_needed);
        check_max_leverage(accRisk.current_leverage, accLim.max_leverage);
        check_max_notional(accRisk.current_exposure, estimated_notional, accLim.max_notional);
        check_max_drawdown(accRisk.daily_realized_pnl, accLim.max_daily_loss);

        return 0; // OK
    }
    // Market data availability + price band check helper
    inline uint32_t check_market_data_and_price_band_helper(
        int64_t best_bid_scaled,
        int64_t best_ask_scaled,
        int64_t price_scaled,
        int64_t max_price_deviation) noexcept
    {
        // Market data yoksa
        if (UNLIKELY(best_bid_scaled == 0 && best_ask_scaled == 0))
            return static_cast<uint32_t>(RiskRejectReason::MarketDataUnavailable);

        // Mid-price
        int64_t mid_price = (best_bid_scaled + best_ask_scaled) / 2;

        // Price deviation kontrolü
        if (UNLIKELY(max_price_deviation > 0 &&
                    std::llabs(price_scaled - mid_price) > max_price_deviation))
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
};

