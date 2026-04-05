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
// DEVELOPER NOTES (PRE-PROFILING):
// - At this stage, frequently-updated atomics are co-located within the same
//   cache line to minimize memory footprint and preserve cache density.
//   After profiling, hot fields may be further separated into distinct
//   cache lines if false sharing is observed and warrants more aggressive
//   cache-line isolation. (SymbolRisk, AccountRisk, OrderRisk)
//
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
struct alignas(64) AccountRisk // May be refactored after profiling to mitigate false sharing
{
    std::atomic<int64_t> current_exposure{0}; 
    std::atomic<int64_t> balance{0};           
    std::atomic<int64_t> used_margin{0};       
    std::atomic<int64_t> current_leverage{1};   
    std::atomic<int64_t> daily_realized_pnl{0};
    std::atomic<int64_t> total_unrealized_pnl{0};
    std::atomic<uint32_t> open_orders_count{0};
    
    std::atomic<AccountStatus> status{AccountStatus::Inactive};
    uint8_t pad1[3]; 
    
    OrderRateLimit orderratelimit;    
    CancelRateLimit cancelratelimit;   
    uint8_t pad2[16]; 
};

// ==========================
// Symbol risk (per-symbol state)
// ==========================
struct alignas(64) SymbolRisk // May be refactored after profiling to mitigate false sharing
{
    // ----- Hot state -----
    std::atomic<int64_t> net_position_scaled{0};     
    std::atomic<int64_t> cost_basis_scaled{0};       
    std::atomic<int64_t> unrealized_pnl{0};          
    std::atomic<int64_t> pending_notional_scaled{0}; 
    std::atomic<int64_t> best_bid{0}; 
    std::atomic<int64_t> best_ask{0}; 
    std::atomic<int32_t> open_orders_count{0};

    uint8_t pad1[12]; 

    OrderRateLimit orderratelimit; 
    CancelRateLimit cancelratelimit; 
    uint8_t pad2[16]; 

    SymbolRisk() noexcept = default;
    SymbolRisk(const SymbolRisk&) = delete;
    SymbolRisk& operator=(const SymbolRisk&) = delete;
    SymbolRisk(SymbolRisk&&) noexcept {}
    SymbolRisk &operator=(SymbolRisk &&) noexcept { return *this; }
};


// ==========================
// Order risk entry (per active order)
// ==========================
struct alignas(64) OrderRisk // May be refactored after profiling to mitigate false sharing
{
    std::atomic<int64_t> price_scaled{0};       
    uint32_t symbol_index{0};      // immutable
    std::atomic<uint32_t> remaining_qty{0};     
    Side side {Side::Unknown};     // immutable
    std::atomic<Status> status{Status::Unknown};
    std::atomic<bool> active{false}; 
    
    uint8_t pad[45]; // alignment padding

    OrderRisk() noexcept = default;
    OrderRisk(const OrderRisk&) = delete;
    OrderRisk& operator=(const OrderRisk&) = delete;
    OrderRisk(OrderRisk&&) noexcept {}
    OrderRisk& operator=(OrderRisk&&) noexcept { return *this; }
};

struct OrderRiskKey {
    uint32_t symbol_index;
    int64_t price_scaled;
    uint8_t side;

    bool operator==(const OrderRiskKey &other) const noexcept 
    {
        return (symbol_index == other.symbol_index) & (price_scaled == other.price_scaled) & (side == other.side);
    }
};

struct OrderRiskHash {
    size_t operator()(const OrderRiskKey &k) const noexcept 
    {  
        uint64_t h = (static_cast<uint64_t>(k.symbol_index) << 48) ^ static_cast<uint64_t>(k.price_scaled << 1) ^ static_cast<uint64_t>(k.side);
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
        : nominal(order.price * static_cast<int64_t>(order.last_exec_quantity)), notional(order.price * static_cast<int64_t>(order.quantity)), 
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

    NotionalOrderValueExceeded = 1 << 4, // Order notional value exceeds per-order notional limit
    MaxNotionalLimitExceeded = 1 << 5,  // Resulting total notional exposure exceeds the maximum allowed limit

    MaxOpenOrdersReachedAccount = 1 << 6, // Maximum number of open orders reached at account level
    MaxOpenOrdersReachedSymbol = 1 << 7, // Maximum number of open orders reached for the symbol

    // Price Validity & Sanity Checks
    PriceTickInvalid = 1 << 8,       // Order price does not conform to the instrument tick size
    InvalidPriceRange = 1 << 9,      // Order price deviates excessively from reference/market price
    FatFingerCheckFailed = 1 << 10,   // Fat-finger protection triggered (abnormal size or price deviation)
    PriceOutsideMarketBand = 1 << 11, // Order price is outside exchange-defined price bands

    // Order Rate & Frequency Limits
    MaxOrderRateLimitExceededAccount = 1 << 12,  // Account-level order submission rate limit exceeded
    MaxCancelRateLimitExceededAccount = 1 << 13, // Account-level cancel rate limit exceeded

    MaxOrderRateLimitExceededSymbol = 1 << 14,  // Symbol-level order submission rate limit exceeded
    MaxCancelRateLimitExceededSymbol = 1 << 15, // Symbol-level cancel rate limit exceeded

    // Self-Trade Prevention & Duplication
    SelfTradeDetected = 1 << 16, // Self-trade detected (crossing orders from same account/strategy)
    DuplicateOrderID = 1 << 17,  // Order ID has already been used

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
    Order &order;
    uint32_t RejectReason;
};

inline constexpr size_t REJECTORDER_QUEUE_SIZE = 1024;
using spscRejectOrderQueue_t = boost::lockfree::spsc_queue<OrderWithRejectReason, boost::lockfree::capacity<REJECTORDER_QUEUE_SIZE>>;

class RiskEngine
{
private:
    static constexpr size_t ORDERRISK_POOL_CAPACITY = 65536; 

    std::array<OrderRisk, ORDERRISK_POOL_CAPACITY> orderrisk_pool_;
    size_t orderrisk_next_slot = 0;

    spscOrderQueue_t &store_to_risk_;
    spscOrderQueue_t &strategy_to_risk_;
    spscRejectOrderQueue_t &risk_to_strategy_;
    spscOrderQueue_t &risk_to_builder_;

    std::array<AccountRisk, VENUE_COUNT> accountrisks_;
    std::array<std::vector<SymbolRisk>, VENUE_COUNT> symbolrisks_;
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
    uint32_t check_price_tick_valid(int64_t price_scaled, const SymbolMeta &symbolmeta) noexcept;

    inline uint32_t check_quantity_bounds(uint32_t qty, uint32_t min_qty, uint32_t max_qty, uint32_t round_lot_size) noexcept
    {
        if (UNLIKELY(qty < min_qty))
            return static_cast<uint32_t>(RiskRejectReason::MinOrderSizeExceeded); 
        if (UNLIKELY(qty > max_qty))
            return static_cast<uint32_t>(RiskRejectReason::MaxOrderSizeExceeded);
        if (UNLIKELY((qty % round_lot_size) != 0))
            return static_cast<uint32_t>(RiskRejectReason::InvalidLotSize);
        return 0;
    }
    
    inline uint32_t check_max_open_orders_account(uint32_t current_open_orders, uint32_t max_open_orders) noexcept
    {
        if (UNLIKELY(current_open_orders >= max_open_orders))
            return static_cast<uint32_t>(RiskRejectReason::MaxOpenOrdersReachedAccount);
        return 0;
    }

    inline uint32_t check_max_open_orders_symbol(uint32_t symbol_open_orders, uint32_t max_symbol_open_orders) noexcept
    {
                                               
        if (UNLIKELY(symbol_open_orders >= max_symbol_open_orders))
            return static_cast<uint32_t>(RiskRejectReason::MaxOpenOrdersReachedSymbol);
        return 0;
    }

    inline uint32_t check_self_trade_order(const auto &orderrisks_, const Order &order, const uint8_t venue_index) noexcept
    {
        OrderRiskKey key{order.symbol_index, order.price, !static_cast<uint8_t>(order.side)};
        auto it = orderrisks_[venue_index].find(key);    

        if(it != orderrisks_[venue_index].end() && it->second->active.load(std::memory_order_acquire))
                    return static_cast<uint32_t>(RiskRejectReason::SelfTradeDetected);
        
        return 0;
    }

    inline uint32_t check_notional_value(int64_t price_scaled, uint32_t qty, int64_t max_notional_scaled) noexcept
    {
        int64_t notional = price_scaled * qty;
        if (UNLIKELY(notional > max_notional_scaled))
            return static_cast<uint32_t>(RiskRejectReason::NotionalOrderValueExceeded);
        return 0;
    }

    inline uint32_t check_max_position_limit(int64_t net_position_scaled, int64_t max_position_scaled) noexcept
    {
        if (UNLIKELY(std::llabs(net_position_scaled) > std::llabs(max_position_scaled)))
            return static_cast<uint32_t>(RiskRejectReason::MaxPositionLimitExceeded);

        return 0;
    }

    inline uint32_t check_account_risk(Order &order, AccountRisk &accRisk, const AccountLimit &accLim, const int64_t price_scaled, const uint32_t qty) noexcept
    {
        uint32_t reason = 0;
        
        auto leverage = accRisk.current_leverage.load(std::memory_order_acquire);
        
        reason |=
            check_acc_status(accRisk.status.load(std::memory_order_acquire)) |
            check_max_leverage(leverage, accLim.max_leverage) |
            check_max_drawdown(accRisk.daily_realized_pnl.load(std::memory_order_acquire), accLim.max_daily_loss);

        if (order.price > 0 && order.order_type == OrderType::Limit)
        {
            auto balance = accRisk.balance.load(std::memory_order_acquire);
            int64_t estimated_notional = price_scaled * qty;
            int64_t estimated_margin_needed = estimated_notional / leverage * 1'000'000ULL; 

            reason |=
                check_free_margin(balance, accRisk.total_unrealized_pnl.load(std::memory_order_acquire), estimated_margin_needed, accRisk.used_margin.load(std::memory_order_acquire)) |
                check_balance(balance, estimated_margin_needed) |
                check_max_notional(accRisk.current_exposure.load(std::memory_order_acquire), estimated_notional, accLim.max_notional);
        }

        return reason; 
    }

    inline uint32_t check_market_data_and_price_band_helper(int64_t best_bid_scaled, int64_t best_ask_scaled, int64_t price_scaled, int64_t max_price_deviation) noexcept
    {
        if (UNLIKELY(best_bid_scaled == 0 && best_ask_scaled == 0))
            return static_cast<uint32_t>(RiskRejectReason::MarketDataUnavailable);

        int64_t mid_price = (best_bid_scaled + best_ask_scaled) / 2;

        if (UNLIKELY(max_price_deviation > 0 && std::llabs(price_scaled - mid_price) > max_price_deviation))
        {
            return static_cast<uint32_t>(RiskRejectReason::PriceOutsideMarketBand);
        }

        return 0; 
    }

    inline uint32_t check_fat_finger_quantity(uint32_t qty, uint32_t fat_finger_qty_threshold) noexcept
    {
        if (UNLIKELY(qty >= fat_finger_qty_threshold))
            return static_cast<uint32_t>(RiskRejectReason::FatFingerCheckFailed);

        return 0;
    }

    inline uint32_t check_fat_finger_price_ratio(int64_t price_scaled, int64_t best_bid_scaled, int64_t best_ask_scaled, int64_t fat_finger_ratio_scaled) noexcept
    {
        int64_t mid_price = (best_bid_scaled + best_ask_scaled) / 2;

        if (UNLIKELY(mid_price > 0))
        {
            int64_t ratio_check = price_scaled / mid_price;
            if (UNLIKELY(ratio_check > fat_finger_ratio_scaled))
                return static_cast<uint32_t>(RiskRejectReason::InvalidPriceRange);
        }

        return 0;
    }

    // Helper functions associated with the other private functions
    void update_unrealized_pnl(SymbolRisk &symRisk, int64_t best_bid, int64_t best_ask) noexcept;
    
    inline uint32_t check_acc_status(AccountStatus status) noexcept
    {
        if (UNLIKELY(status != AccountStatus::Active))
            return static_cast<uint32_t>(RiskRejectReason::RestrictedAccountStatus);

        return 0;
    }

    inline uint32_t check_free_margin(const int64_t balance, const int64_t total_unrealized_pnl, const __int128 &estimated_margin_needed, const int64_t used_margin) noexcept
    {
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

    inline uint32_t check_max_leverage(const int64_t current_leverage, const int64_t max_leverage) noexcept
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

    
};

