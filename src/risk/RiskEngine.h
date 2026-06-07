// ======================================================================================
// RiskEngine
//
// PURPOSE:
// - Central real-time risk evaluation and state update engine.
// - Performs pre-trade and post-trade risk checks with deterministic,
//   constant-time behavior under multi-threaded execution.
//
// THREAD MODEL:
// - Risk update thread: processes executed orders and market data updates.
// - Risk check thread: validates new orders before routing.
// - These threads NEVER block each other.
//
// THREAD SAFETY:
// - Lock-free design.
// - All shared mutable state uses std::atomic with explicit memory ordering.
// - No mutexes, spinlocks, or condition variables are used.
//
// PERFORMANCE & DESIGN NOTES:
// - Hot-path functions are explicitly inlined to eliminate call overhead.
// - Risk checks rely on simple arithmetic and branch-predictable conditions.
// - Order self-trade prevention uses hash-based lookup with preallocated pools.
// - Rate limiting uses rdtsc-based timing for nanosecond-level resolution.
// - Cache-line isolation is applied selectively using alignas(64).
// - AccountRisk, SymbolRisk, and OrderRisk are cache-line aligned.
// - Manual padding is intentionally used to prevent false sharing.
// - Read-only configuration (Limits) is kept separate from mutable risk state.
//
// DESIGN ASSUMPTIONS:
// - One account per venue (by design, not a limitation).
// - Symbol indices are pre-resolved via HashTables.
// - All quantities are stored in scaled integer form (no floating point).
//
// DEVELOPER NOTES:
// - The current implementation assumes a single account per venue for risk-limits
//   evaluation. While other parts of the system are designed with multi-account
//   support in mind, this class intentionally models one account per venue to
//   keep risk checks simple and fast. The structure can be extended to support
//   multiple accounts per venue if required in future engine revisions.
//
// ======================================================================================

#pragma once

#include "Order.h"
#include "Protocol-Venue.h"
#include "common.h"
#include "Limits.h"

#include <array>
#include <cstdint>
#include <cstddef>
#include <vector>
#include <atomic>
#include <x86intrin.h>

#include <boost/lockfree/spsc_queue.hpp>
#include <absl/container/flat_hash_map.h>

class OrderManager;
struct SymbolMeta;
class MarketBook;
class HashTables;

// ==========================
// Order rate limit bucket
// ==========================
struct OrderRateLimit
{
    uint64_t tokens;             // remaining tokens
    uint64_t last_refill_ns;     // last refill timestamp
    uint64_t capacity;           // max tokens (config, immutable)
    uint64_t refill_interval_ns; // refill interval in ns (config, immutable)
};

// ==========================
// Cancel rate limit bucket
// ==========================
struct CancelRateLimit
{
    uint64_t last_cancel_timestamp_ns{0}; 
    uint32_t cancel_count_window{0};
    uint32_t max_cancels_per_sec{0};  // config, immutable
};

// ==========================
// Account status enum
// ==========================
enum AccountStatus : uint8_t
{
    Inactive = 0,
    Active = 1,
    Suspended = 2,
    Closed = 3
};

// ==========================
// Account risk (per-venue/account)
// ==========================
struct AccountRisk // All fields are seperated due to high possibility of false sharing, currently we have only one account per venue, so we can afford this much space for better performance, may be refactored after profiling
{
    alignas(64) std::atomic<int64_t> current_exposure{0}; 
    alignas(64) std::atomic<int64_t> balance{10'000'000};
    alignas(64) std::atomic<int64_t> positional_exposure{0};            
    alignas(64) std::atomic<int64_t> used_margin{0};       
    alignas(64) std::atomic<int64_t> current_leverage{1};   
    alignas(64) std::atomic<int64_t> daily_realized_pnl{0};
    alignas(64) std::atomic<int64_t> total_unrealized_pnl{0};
    alignas(64) std::atomic<uint32_t> open_orders_count{0};
    alignas(64) std::atomic<AccountStatus> status{AccountStatus::Active}; // Inactive after tests
    
    alignas(64)
    OrderRateLimit orderratelimit;    
    CancelRateLimit cancelratelimit;   
    uint8_t pad[16]; 

    AccountRisk() noexcept = default;
    AccountRisk(const AccountRisk&) = delete;
    AccountRisk& operator=(const AccountRisk&) = delete;
    AccountRisk(AccountRisk&&) = delete;
    AccountRisk& operator=(AccountRisk &&) = delete;
};

// ==========================
// Symbol risk (per-symbol state)
// ==========================
struct SymbolRisk // Fields are grouped by similar access patterns to optimize cache usage and minimize false sharing; may be refactored after profiling
{
    // Cache line 1 — written on every order (New/Cancel/Replace...)
    alignas(64)
    std::atomic<int64_t> pending_notional_scaled{0};
    std::atomic<int32_t> open_orders_count{0};
    uint8_t pad1[52];

    // Cache line 2 — written on fill (Partial/Filled/Cancelled...) and ITCH updates(best bid/ask, unrealized PnL)
    alignas(64)
    std::atomic<int64_t> best_bid{0};
    std::atomic<int64_t> best_ask{0};
    int64_t unrealized_pnl{0};
    int64_t cost_basis_scaled{0};
    int64_t realized_pnl{0};
    std::atomic<int64_t> net_position{0};
    std::atomic<int32_t> avg_entry_price{0};
    uint8_t pad2[12];

    // Cache line 3 — rate limiter, check thread only
    alignas(64) 
    OrderRateLimit orderratelimit;
    CancelRateLimit cancelratelimit;
    uint8_t pad4[16];

    SymbolRisk() noexcept = default;
    SymbolRisk(const SymbolRisk&) = delete;
    SymbolRisk& operator=(const SymbolRisk&) = delete;
    SymbolRisk(SymbolRisk&&) = delete;
    SymbolRisk& operator=(SymbolRisk &&) = delete;
};


// ==========================
// Order risk entry (per active order)
// ==========================
struct alignas(64) OrderRisk // May be refactored after profiling to mitigate false sharing
{
    std::atomic<int64_t> price{0};       
    uint32_t symbol_index{0};      // immutable
    std::atomic<uint32_t> remaining_qty{0};     
    Side side {Side::Unknown};     // immutable
    std::atomic<Status> status{Status::Unknown};
    std::atomic<bool> active{false};
    Venue venue{Venue::Unknown};     // immutable 
    uint8_t pad1[44];

    OrderRisk() noexcept = default;
    OrderRisk(const OrderRisk&) = delete;
    OrderRisk& operator=(const OrderRisk&) = delete;
    OrderRisk(OrderRisk&&) = delete;
    OrderRisk& operator=(OrderRisk&&) = delete;
};

struct OrderRiskKey {
    int64_t price;
    uint32_t symbol_index;
    uint8_t side;
    uint8_t venue;

    bool operator==(const OrderRiskKey &other) const noexcept 
    {
        return (symbol_index == other.symbol_index) & (price == other.price) & (side == other.side) & (venue == other.venue);
    }
};

struct OrderRiskHash {
    size_t operator()(const OrderRiskKey &k) const noexcept 
    {  
        uint64_t h = (static_cast<uint64_t>(k.symbol_index) << 48) | static_cast<uint64_t>(k.price << 16) | static_cast<uint64_t>(k.side << 8) | static_cast<uint64_t>(k.venue);
        return absl::Hash<uint64_t>()(h);
    }
};

struct OrderMetrics
{
    const int64_t nominal;
    const int64_t notional;
    const int64_t replaced;
    const int8_t side_mult;
    
   
    OrderMetrics(const Order &order) noexcept
        : nominal(order.price * static_cast<int64_t>(order.last_exec_quantity)), 
          notional(order.price * static_cast<int64_t>(order.quantity)), 
          replaced(order.price * static_cast<int64_t>(order.replaced_quantity)),
          side_mult(order.side == Side::Buy ? 1 : -1) {}
};

enum class RiskRejectReason : uint32_t
{
    Unknown = 0,

    // Position & Exposure Limits
    MaxPositionLimitExceeded = 1 << 0, // Resulting position exceeds the configured maximum position limit
    MaxOrderSizeExceeded = 1 << 1,     // Order quantity exceeds the maximum allowed order size (lot-based)
    MinOrderSizeExceeded = 1 << 2,    // Order quantity is below the minimum allowed order size (lot-based)
    InvalidLotSize = 1 << 3,          // Order quantity does not conform to the required lot / step size

    MaxOrderNotionalLimitExceeded = 1 << 4, // Order notional value exceeds per-order notional limit
    MaxNotionalLimitExceeded = 1 << 5,  // Resulting total notional exposure exceeds the maximum allowed limit
    MaxSymbolExposureLimitExceeded = 1 << 6, // Order price is outside exchange-defined price bands

    MaxOpenOrdersReachedAccount = 1 << 7, // Maximum number of open orders reached at account level
    MaxOpenOrdersReachedSymbol = 1 << 8, // Maximum number of open orders reached for the symbol

    // Price Validity & Sanity Checks
    PriceTickInvalid = 1 << 9,       // Order price does not conform to the instrument tick size
    InvalidPriceRange = 1 << 10,      // Order price deviates excessively from reference/market price
    FatFingerCheckFailed = 1 << 11,   // Fat-finger protection triggered (abnormal size or price deviation)
    
    // Order Rate & Frequency Limits
    MaxOrderRateLimitExceededAccount = 1 << 12,  // Account-level order submission rate limit exceeded
    MaxCancelRateLimitExceededAccount = 1 << 13, // Account-level cancel rate limit exceeded

    MaxOrderRateLimitExceededSymbol = 1 << 14,  // Symbol-level order submission rate limit exceeded
    MaxCancelRateLimitExceededSymbol = 1 << 15, // Symbol-level cancel rate limit exceeded

    // Self-Trade Prevention & Duplication
    SelfTradeDetected = 1 << 16, // Self-trade detected (crossing orders from same account/strategy)
    // DuplicateOrderID = 1 << 17,  // Order ID has already been used

    // Account & Margin Constraints
    RestrictedAccountStatus = 1 << 18,    // Account is restricted, suspended, or not authorized for trading
    AccountBalanceInsufficient = 1 << 19, // Insufficient account balance for the order
    MaxLeverageExceeded = 1 << 20,        // Order would exceed the maximum allowed leverage
    FreeMarginInsufficient = 1 << 21,     // Insufficient free margin after considering the order

    // Loss Limits
    RealizedLossLimitExceeded = 1 << 22,   // Realized loss limit (daily/periodic) exceeded
    UnrealizedLossLimitExceeded = 1 << 23, // Unrealized loss limit exceeded

    // Market / Venue State
    VenueTradingHalted = 1 << 24,      // Trading is halted at venue level
    CircuitBreakerTriggered = 1 << 25, // Internal circuit breaker triggered
    SymbolTradingHalted = 1 << 26,     // Trading is halted for the specific symbol

    // Technical / Infrastructure
    MarketDataUnavailable = 1 << 27,  // Required market data unavailable (price checks cannot be performed)
    RiskEngineInternalError = 1 << 28 // Unexpected internal risk engine error
};

struct OrderWithRejectReason
{
    Order *order = nullptr;
    uint32_t RejectReason = 0;
};

inline constexpr size_t REJECTORDER_QUEUE_SIZE = 1024;
using spscRejectOrderQueue_t = boost::lockfree::spsc_queue<OrderWithRejectReason, boost::lockfree::capacity<REJECTORDER_QUEUE_SIZE>>;

class RiskEngine
{
public: /// Debugging and monitoring fields. Will be removed after profiling and debugging.
    alignas(64) std::atomic<uint64_t> pipeline_seq{0}; // Used for debugging and monitoring the processing sequence of orders through the pipeline. Will be removed after profiling and debugging.
    char pad[56];
    
private:
public: /// Debugging and monitoring fields. Will be removed after profiling and debugging.
    static constexpr size_t ORDERRISK_POOL_CAPACITY = 65536;
    static constexpr size_t MAX_SYMBOL_COUNT = 512; 

    std::array<OrderRisk, ORDERRISK_POOL_CAPACITY> orderrisk_pool_;
    size_t orderrisk_next_slot = 0;

    spscOrderQueue_t &store_to_risk_;
    spscOrderQueue_t &strategy_to_risk_;
    spscRejectOrderQueue_t &risk_to_strategy_;
    spscOrderQueue_t &risk_to_builder_;

    std::array<AccountRisk, VENUE_COUNT> accountrisks_;
    std::array<std::array<SymbolRisk, MAX_SYMBOL_COUNT>, VENUE_COUNT> symbolrisks_;
    std::array<absl::flat_hash_map<OrderRiskKey, OrderRisk*, OrderRiskHash>, VENUE_COUNT> orderrisks_;

    HashTables &hashtables_;
    MarketBook &marketbook_;
    Limits &limits_;
    OrderManager &ord_mngr_;

public:
    RiskEngine(spscOrderQueue_t &store_to_risk, spscOrderQueue_t &strategy_to_risk, spscRejectOrderQueue_t &risk_to_strategy, spscOrderQueue_t &risk_to_builder, HashTables &hashtables, MarketBook &marketbook, Limits &limits, OrderManager &ord_mngr) noexcept;

    bool update_risk() noexcept;
    bool check_risk() noexcept;
   
private:
    // Helper functions associated with the public functions
    // --------------------------------------INITIALIZE FUNCTIONS-----------------------------------------------
    void initialize_accountrisks() noexcept;
    void initialize_symbolrisks() noexcept;
    
    // --------------------------------------UPDATE FUNCTIONS-----------------------------------------------
    void update_order_risk(const Order &order, const uint8_t venue_index) noexcept;
    bool update_risk_for_protocol_fix(AccountRisk &accRisk, const AccountLimit &accLim, Order &order, const OrderMetrics metrics) noexcept;
    void update_symbol_risk(AccountRisk &accRisk, const Order &order, const OrderMetrics metrics, const uint8_t venue_index) noexcept;
    void update_account_risk(AccountRisk &accRisk, const AccountLimit &accLim, const Order &order, const OrderMetrics metrics) noexcept;
    
    // --------------------------------------CHECK FUNCTIONS-----------------------------------------------
    bool check_venue_halt_and_circuit(Order &order, const SymbolMeta &symbolmeta) noexcept;
    bool check_order_rate_limit(AccountRisk &accRisk, SymbolRisk &symRisk, Order &order) noexcept;
    bool check_cancel_rate_limit(AccountRisk &accRisk, SymbolRisk &symRisk, Order &order) noexcept;
    uint32_t check_price_tick_valid(const int64_t price, const SymbolMeta &symbolmeta) noexcept;

    inline uint32_t check_quantity_bounds(const uint32_t qty, const uint32_t min_qty, const uint32_t max_qty, const uint32_t round_lot_size) noexcept
    {
        if (UNLIKELY(qty < min_qty))
            return static_cast<uint32_t>(RiskRejectReason::MinOrderSizeExceeded); 
        if (UNLIKELY(qty > max_qty))
            return static_cast<uint32_t>(RiskRejectReason::MaxOrderSizeExceeded);
        if (UNLIKELY((qty % round_lot_size) != 0))
            return static_cast<uint32_t>(RiskRejectReason::InvalidLotSize);
        return 0;
    }
    
    inline uint32_t check_max_open_orders_account(const uint32_t current_open_orders, const uint32_t max_open_orders) noexcept
    {
        if (UNLIKELY(current_open_orders >= max_open_orders))
            return static_cast<uint32_t>(RiskRejectReason::MaxOpenOrdersReachedAccount);
        return 0;
    }

    inline uint32_t check_max_open_orders_symbol(const uint32_t symbol_open_orders, const uint32_t max_symbol_open_orders) noexcept
    {
                                               
        if (UNLIKELY(symbol_open_orders >= max_symbol_open_orders))
            return static_cast<uint32_t>(RiskRejectReason::MaxOpenOrdersReachedSymbol);
        return 0;
    }

    inline uint32_t check_self_trade_order(const Order &order, const uint8_t venue_index) noexcept
    {
        OrderRiskKey key{order.symbol_index, order.price, !static_cast<uint8_t>(order.side), venue_index};
        auto it = orderrisks_[venue_index].find(key);    
    
        if(it != orderrisks_[venue_index].end() && it->second->active.load(std::memory_order_acquire))
                    return static_cast<uint32_t>(RiskRejectReason::SelfTradeDetected);
        
        return 0;
    }

    inline uint32_t check_max_order_notional_value(const int64_t price, const uint32_t qty, const int64_t max_notional_scaled) noexcept
    {
        int64_t notional = price * qty;
        if (UNLIKELY(notional > max_notional_scaled))
            return static_cast<uint32_t>(RiskRejectReason::MaxOrderNotionalLimitExceeded);
        return 0;
    }

    inline uint32_t check_max_symbol_exposure_value(const int64_t price, const uint32_t qty, const Side side, const int64_t pending_notional_scaled, const int64_t max_exposure_scaled) noexcept
    {
        const int8_t side_mult = (side == Side::Buy) ? 1 : -1;
        int64_t notional = price * qty * side_mult;
        if (UNLIKELY(pending_notional_scaled + notional > max_exposure_scaled))
            return static_cast<uint32_t>(RiskRejectReason::MaxSymbolExposureLimitExceeded);
        return 0;
    }

    inline uint32_t check_max_position_limit(const int64_t net_position_scaled, const int64_t order_qty, const Side side, const int64_t max_position_scaled) noexcept
    {
        const auto side_mult = (side == Side::Buy) ? 1 : -1;
        const int64_t projected_position = net_position_scaled + (side_mult * order_qty);

        if (UNLIKELY(std::llabs(projected_position) > std::llabs(max_position_scaled)))
            return static_cast<uint32_t>(RiskRejectReason::MaxPositionLimitExceeded);
        return 0;
    }

    inline uint32_t check_market_data_and_deviation(const int64_t best_bid_scaled, const int64_t best_ask_scaled, const int64_t price, const int64_t max_price_deviation) noexcept
    {
        if (UNLIKELY(best_bid_scaled == 0 && best_ask_scaled == 0))
            return static_cast<uint32_t>(RiskRejectReason::MarketDataUnavailable);

        int64_t mid_price = (best_bid_scaled + best_ask_scaled) / 2;

        if (UNLIKELY(max_price_deviation > 0 && std::llabs(price - mid_price) > max_price_deviation))
        {
            return static_cast<uint32_t>(RiskRejectReason::InvalidPriceRange);
        }

        return 0; 
    }

    inline uint32_t check_fat_finger(const uint32_t qty, const uint32_t fat_finger_qty_threshold, const int64_t price, const int64_t best_bid_scaled, const int64_t best_ask_scaled, const int64_t fat_finger_ratio_scaled) noexcept
    {
        int64_t mid = (best_bid_scaled + best_ask_scaled) / 2;

        if (LIKELY(mid > 0))
        {
            int64_t ratio = (price * 1000) / mid;

            if (UNLIKELY(qty >= fat_finger_qty_threshold && ratio > fat_finger_ratio_scaled))
                return static_cast<uint32_t>(RiskRejectReason::FatFingerCheckFailed);
        }
        
        return 0;
    }

    inline uint32_t check_account_risk(const Order &order, const AccountRisk &accRisk, const AccountLimit &accLim, SymbolRisk &symRisk, const int64_t price, const uint32_t qty) noexcept
    {
        uint32_t reason = 0;

        const int64_t balance              = accRisk.balance.load(std::memory_order_acquire);
        const int64_t current_exposure     = accRisk.current_exposure.load(std::memory_order_acquire);
        const int64_t used_margin          = accRisk.used_margin.load(std::memory_order_acquire);
        const int64_t total_unrealized_pnl = accRisk.total_unrealized_pnl.load(std::memory_order_acquire);
        const int64_t daily_realized_pnl   = accRisk.daily_realized_pnl.load(std::memory_order_acquire);
        const int64_t avg                  = symRisk.avg_entry_price.load(std::memory_order_acquire);
        const int64_t best_bid             = symRisk.best_bid.load(std::memory_order_acquire);
        const int64_t estimated_notional   = price * static_cast<int64_t>(qty);

        reason |= check_acc_status(accRisk.status.load(std::memory_order_acquire));
        reason |= check_max_leverage(balance, current_exposure, estimated_notional, accLim.max_leverage);
        reason |= check_max_drawdown(daily_realized_pnl, avg, price, qty, order.side, accLim.max_daily_loss);
        reason |= check_unrealized_loss(total_unrealized_pnl, best_bid, avg, price, qty, order.side, accLim.max_unrealized_loss);

        if (order.price > 0 && order.order_type == OrderType::Limit)
        {
            reason |= check_free_margin(balance, total_unrealized_pnl, estimated_notional, used_margin);
            reason |= check_balance(balance, estimated_notional);
            reason |= check_max_notional(current_exposure, estimated_notional, accLim.max_notional);
        }

        return reason;
    }

    // Helper functions associated with the other private functions
    void update_unrealized_pnl(SymbolRisk &symRisk, const int64_t best_bid, const int64_t best_ask) noexcept;
    

    inline uint32_t check_acc_status(AccountStatus status) noexcept
    {
        if (UNLIKELY(status != AccountStatus::Active))
            return static_cast<uint32_t>(RiskRejectReason::RestrictedAccountStatus);

        return 0;
    }

    inline uint32_t check_free_margin(const int64_t balance, const int64_t total_unrealized_pnl, const int64_t estimated_notional, const int64_t used_margin) noexcept
    {
        const int64_t equity = balance + total_unrealized_pnl;
        const int64_t free_margin = equity - used_margin;

        if (UNLIKELY(free_margin < estimated_notional))
            return static_cast<uint32_t>(RiskRejectReason::FreeMarginInsufficient);
        
        return 0;
    }

    inline uint32_t check_balance(const int64_t balance, const int64_t estimated_notional) noexcept
    {
        if (UNLIKELY(balance < estimated_notional))
            return static_cast<uint32_t>(RiskRejectReason::AccountBalanceInsufficient);
        
        return 0;
    }

    inline uint32_t check_max_leverage(const int64_t balance, const int64_t current_exposure, const int64_t estimated_notional, const int64_t max_leverage) noexcept
    {
        if (UNLIKELY(balance <= 0))
            return static_cast<uint32_t>(RiskRejectReason::MaxLeverageExceeded);

        const int64_t projected_leverage = (current_exposure + estimated_notional) * 10000 / balance;

        if (UNLIKELY(projected_leverage > max_leverage))
            return static_cast<uint32_t>(RiskRejectReason::MaxLeverageExceeded);
        
        return 0;
    }

    inline uint32_t check_max_notional(const int64_t current_exposure, const int64_t estimated_notional, const int64_t max_notional) noexcept
    {
        if (UNLIKELY((current_exposure + estimated_notional) > max_notional))
            return static_cast<uint32_t>(RiskRejectReason::MaxNotionalLimitExceeded);
        return 0;
    }

    inline uint32_t check_max_drawdown(const int64_t daily_realized_pnl, const int64_t avg, const int64_t price, const uint32_t qty, const Side side, const int64_t max_daily_loss) noexcept
    {
        const int64_t projected_realized = daily_realized_pnl + (side == Side::Sell 
            ? (price - avg) * static_cast<int64_t>(qty) 
            : 0);

        if (UNLIKELY(projected_realized < -max_daily_loss))
            return static_cast<uint32_t>(RiskRejectReason::RealizedLossLimitExceeded);
        
        return 0;
    }

    inline uint32_t check_unrealized_loss(const int64_t total_unrealized_pnl, const int64_t best_bid, const int64_t avg, const int64_t price, const uint32_t qty, const Side side, const int64_t max_unrealized_loss) noexcept
    {
        const int64_t projected_unrealized = side == Side::Buy
            ? total_unrealized_pnl + (best_bid - price) * static_cast<int64_t>(qty)
            : total_unrealized_pnl - (best_bid - avg) * static_cast<int64_t>(qty);

        if (UNLIKELY(projected_unrealized < -max_unrealized_loss))
            return static_cast<uint32_t>(RiskRejectReason::UnrealizedLossLimitExceeded);
        
        return 0;
    }    
};

