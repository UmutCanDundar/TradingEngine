#pragma once

#include "Order.h"
#include "Protocol-Venue.h"
#include "HashTables.h"
#include "common.h"
#include "MarketBook.h"
#include "Limits.h"
#include "Store_RAM.h"

#include <cstring>
#include <boost/lockfree/spsc_queue.hpp>
#include <array>
#include <cstdint>
#include <cstddef>
#include <vector>
#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>
#include <x86intrin.h>
#include <atomic>

// ==========================
// Order rate limit bucket
// ==========================
struct OrderRateLimit
{
    uint64_t tokens;             // remaining tokens
    uint64_t last_refill_ns;     // last refill timestamp
    uint64_t capacity;                        // max tokens (config, immutable)
    uint64_t refill_interval_ns;              // refill interval in ns (config, immutable)
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
struct alignas(64) AccountRisk
{
    std::atomic<int64_t> current_exposure{0}; 
    std::atomic<int64_t> balance{0};           
    std::atomic<int64_t> used_margin{0};       
    std::atomic<int64_t> current_leverage{0};   
    std::atomic<int64_t> daily_realized_pnl{0};
    std::atomic<int64_t> total_unrealized_pnl{0};
    std::atomic<uint32_t> open_orders_count{0};
    
    std::atomic<AccountStatus> status{AccountStatus::Inactive};
    uint8_t pad1[3]; // enum alignment
    
    OrderRateLimit orderratelimit;     // rate limit bucket
    CancelRateLimit cancelratelimit;   // rate limit bucket
    uint8_t pad2[16]; // alignment padding
};

// ==========================
// Symbol risk (per-symbol state)
// ==========================
struct alignas(64) SymbolRisk
{
    // ----- Hot state -----
    std::atomic<int64_t> net_position_scaled{0};     
    std::atomic<int64_t> cost_basis_scaled{0};       
    std::atomic<int64_t> unrealized_pnl{0};          
    std::atomic<int64_t> pending_notional_scaled{0}; 
    std::atomic<int64_t> best_bid{0}; 
    std::atomic<int64_t> best_ask{0}; 
    std::atomic<uint32_t> open_orders_count{0};

    uint8_t pad1[12]; // alignment padding

    OrderRateLimit orderratelimit; 
    CancelRateLimit cancelratelimit; 
    uint8_t pad2[16]; // alignment padding

    SymbolRisk() noexcept = default;
    SymbolRisk(const SymbolRisk&) = delete;
    SymbolRisk& operator=(const SymbolRisk&) = delete;
    SymbolRisk(SymbolRisk&& other) noexcept {}
    SymbolRisk& operator=(SymbolRisk&& other) noexcept { return *this; }
};

// ==========================
// Order risk entry (per active order)
// ==========================
struct alignas(64) OrderRisk
{
    std::atomic<int64_t> price_scaled{0};       
    uint32_t symbol_index{0};      // immutable
    std::atomic<uint32_t> remaining_qty{0};     
    Side side {Side::Unknown};                     // immutable
    std::atomic<Status> status{Status::Unknown};
    std::atomic<bool> active{false}; 
    
    uint8_t pad[45]; // alignment padding

    OrderRisk() noexcept = default;
    OrderRisk(const OrderRisk&) = delete;
    OrderRisk& operator=(const OrderRisk&) = delete;
    OrderRisk(OrderRisk&& other) noexcept {}
    OrderRisk& operator=(OrderRisk&& other) noexcept { return *this; }
};

struct OrderRiskKey {
    uint32_t symbol_index;
    int64_t price_scaled;
    uint8_t side;

    bool operator==(const OrderRiskKey &other) const noexcept 
    {
        return (symbol_index == other.symbol_index) & (price_scaled == other.price_scaled) & side == other.side;
    }
};

struct OrderRiskHash {
    size_t operator()(const OrderRiskKey &k) const noexcept 
    {  
        uint64_t h = (static_cast<uint64_t>(k.symbol_index) << 48) ^ static_cast<uint64_t>(k.price_scaled << 1) ^ static_cast<uint64_t>(k.side);
        return absl::Hash<uint64_t>()(h);
    }
};

enum class RiskRejectReason : uint32_t
{
    Unknown = 0, // Tanımsız ret sebebi
    // Position & exposure limits
    MaxPositionLimitExceeded = 1 << 0, // Belirlen pozisyon limitinin aşılması
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
    SymbolTradingHalted = 1 << 28,

    // Technical
    MarketDataUnavailable = 1 << 21,  // Market verisi yok (price check yapılamıyor)
    RiskEngineInternalError = 1 << 22 // Beklenmeyen risk engine hatası

};

struct OrderWithRejectReason
{
    Order &order;
    uint32_t RejectReason;
};

inline constexpr size_t REJECTORDER_QUEUE_SIZE = 1024;

using spscRejectOrderQueue_t = boost::lockfree::spsc_queue<OrderWithRejectReason, boost::lockfree::capacity<REJECTORDER_QUEUE_SIZE>>;
using spscOrderQueue_t = boost::lockfree::spsc_queue<Order *, boost::lockfree::capacity<ORDER_QUEUE_CAPACITY>>;

class RiskEngine
{
private:
    
    std::array<OrderRisk, ORDER_POOL_CAPACITY> orderrisk_pool_;
    size_t orderrisk_next_slot = 0;

    spscOrderQueue_t &store_to_risk_;
    spscOrderQueue_t &strategy_to_risk_;
    spscRejectOrderQueue_t &risk_to_strategy_;
    spscOrderQueue_t &risk_to_builder_;

    std::array<AccountRisk, VENUE_COUNT> accountrisks_;
    std::array<std::vector<SymbolRisk>, VENUE_COUNT> symbolrisks_;
    std::array<absl::flat_hash_map<OrderRiskKey, OrderRisk*, OrderRiskHash>, VENUE_COUNT> orderrisks_;

    // std::array<absl::flat_hash_set<uint64_t>, VENUE_COUNT> order_ids; 

    HashTables &hashtables_;
    MarketBook &marketbook_;
    Limits &limits_;
    Store_RAM &store_ram_;

public:
    RiskEngine(spscOrderQueue_t &store_to_risk, spscOrderQueue_t &strategy_to_risk, spscRejectOrderQueue_t &risk_to_strategy, spscOrderQueue_t &risk_to_builder, HashTables &hashtables, MarketBook &marketbook, Limits &limits, Store_RAM &store_ram) noexcept;

    void update_risk() noexcept;
    void check_risk() noexcept;
   
private: // Helper functions associated with the other private functions

    inline void update_unrealized_pnl(SymbolRisk &symRisk) noexcept
    {
        const int64_t net_pos = symRisk.net_position_scaled.load(std::memory_order_relaxed);
        
        if (net_pos == 0)
        {
            symRisk.unrealized_pnl.store(0, std::memory_order_release);
            return;
        }

        const int64_t avg_entry_price = symRisk.cost_basis_scaled.load(std::memory_order_relaxed) / symRisk.net_position_scaled.load(std::memory_order_relaxed);
        const int64_t position_abs = std::abs(symRisk.net_position_scaled.load(std::memory_order_relaxed));

        if (net_pos > 0)
        {
            // Long pozisyon → çıkış bid üzerinden
            const int64_t pnl = (symRisk.best_bid.load(std::memory_order_relaxed) - avg_entry_price) * position_abs;
            symRisk.unrealized_pnl.store(pnl, std::memory_order_release);
        }
        else
        {
            // Short pozisyon → çıkış ask üzerinden
            const int64_t pnl = (avg_entry_price - symRisk.best_ask.load(std::memory_order_relaxed)) * position_abs;
            symRisk.unrealized_pnl.store(pnl, std::memory_order_release);
        }
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


private:  // Helper functions associated with the public functions

    // --------------------------------------INITIALIZE FUNCTIONS-----------------------------------------------
    void initialize_accountrisks() noexcept;
    void initialize_symbolrisks() noexcept;
    
    // --------------------------------------UPDATE FUNCTIONS-----------------------------------------------
    void update_order_risk(const Order &order, const uint8_t venue_index) noexcept;
    bool update_risk_for_protocol_fix(AccountRisk &accRisk, const AccountLimit &accLim, Order &order) noexcept;
    
    inline void update_symbol_risk(AccountRisk &accRisk, const Order &order, const uint8_t venue_index) noexcept
    {
        auto &symRisk = symbolrisks_[venue_index][order.symbol_index];

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
                symRisk.open_orders_count.fetch_add(1, std::memory_order_acq_rel);
                symRisk.pending_notional_scaled.fetch_add(order.price * static_cast<int64_t>(order.quantity), std::memory_order_release);
                break;

            // --- Parsiyel dolum ---
            case Status::Partial:
            {
                int8_t side_mult = (order.side == Side::Buy ? 1 : -1);

                symRisk.net_position_scaled.fetch_add(side_mult * filled_qty_int64, std::memory_order_release);
                symRisk.pending_notional_scaled.fetch_sub(filled_qty_int64 * order.price, std::memory_order_release);
                symRisk.cost_basis_scaled.fetch_add(side_mult * (order.price * filled_qty_int64), std::memory_order_release);
                break;
            }
            // --- Tam dolum ---
            case Status::Filled:
            {
                int8_t side_mult = (order.side == Side::Buy ? 1 : -1);

                symRisk.net_position_scaled.fetch_add(side_mult * filled_qty_int64, std::memory_order_release);
                symRisk.pending_notional_scaled.fetch_sub(filled_qty_int64 * order.price, std::memory_order_release);
                symRisk.cost_basis_scaled.fetch_add(side_mult * (order.price * filled_qty_int64), std::memory_order_release);
                symRisk.open_orders_count.fetch_sub(1, std::memory_order_release);
                break;
            }
            case Status::Cancelled:
            case Status::Expired:
            case Status::DoneForDay:
            case Status::Stopped:
            case Status::Suspended:
                symRisk.open_orders_count.fetch_sub(1, std::memory_order_release);
                symRisk.pending_notional_scaled.store(0, std::memory_order_release);
                break;

            case Status::Rejected:          // Order hiç yaşamadı
            case Status::CancelReject:      // Cancel reddedildi, order hala aktif
            case Status::ReplaceReject:     // Replace reddedildi, order hala aktif
            case Status::PendingCancel:     // Ara durum
            case Status::PendingReplace:    // Ara durum
            case Status::Unknown:           // Belirsiz
            case Status::Restated:
            default:
                break;
            }
        }
        // ----- Başkasına ait market verisi -----
        else
        {
            symRisk.best_bid.store(marketbook_.best_bid(marketbook_.get_symBook(order)), std::memory_order_release);
            symRisk.best_ask.store(marketbook_.best_ask(marketbook_.get_symBook(order)), std::memory_order_release);
            accRisk.total_unrealized_pnl.fetch_sub(symRisk.unrealized_pnl.load(std::memory_order_relaxed),std::memory_order_release);
            update_unrealized_pnl(symRisk);
            accRisk.total_unrealized_pnl.fetch_add(symRisk.unrealized_pnl.load(std::memory_order_relaxed), std::memory_order_release);
        }

        // Güvenlik: negatif değerleri engelle
        if (UNLIKELY(symRisk.pending_notional_scaled.load(std::memory_order_relaxed) < 0))
            symRisk.pending_notional_scaled.store(0, std::memory_order_release);
        if (UNLIKELY(symRisk.open_orders_count.load(std::memory_order_relaxed) < 0))
            symRisk.open_orders_count.store(0, std::memory_order_release);
    }

    inline void update_account_risk(AccountRisk &accRisk, const AccountLimit &accLim, const Order &order, const uint8_t venue_index) noexcept
    {
        const int8_t side_mult = (order.side == Side::Buy ? 1 : -1);
        const int64_t nominal = order.price * static_cast<int64_t>(order.last_exec_quantity);
        const int64_t fee = order.order_type == OrderType::Market ? accLim.taker_fee_rate * nominal / 1'000'000 : accLim.maker_fee_rate * nominal / 1'000'000;

        switch (order.status) {

            // --- Emir gönderildi / kabul edildi (blokaj başlar) ---
            case Status::New:
            case Status::PendingNew:
            case Status::Accepted:
            case Status::PendingSubmit: {
                accRisk.current_exposure.fetch_add(nominal * side_mult, std::memory_order_release);
                accRisk.used_margin.fetch_add(nominal, std::memory_order_release);
                accRisk.open_orders_count.fetch_add(1, std::memory_order_release);
                break;
            }

            // --- Parsiyel dolum ---
            case Status::Partial: {
                accRisk.balance.fetch_sub(nominal + fee, std::memory_order_release); 
                break;
            }

            // --- Tam dolum ---
            case Status::Filled: {
                accRisk.balance.fetch_sub(nominal + fee, std::memory_order_release); 
                accRisk.used_margin.fetch_sub(nominal, std::memory_order_release);
                accRisk.open_orders_count.fetch_sub(1, std::memory_order_release);
                break;
            }

            // --- İptal veya reddedilme (blokaj geri çözülür) ---
            case Status::Cancelled:
            case Status::Expired:
            case Status::DoneForDay:
            case Status::Stopped:
            case Status::Rejected:
            case Status::CancelReject:
            case Status::ReplaceReject: {
                accRisk.current_exposure.fetch_sub(nominal * side_mult, std::memory_order_release);
                accRisk.used_margin.fetch_sub(nominal, std::memory_order_release);
                accRisk.open_orders_count.fetch_sub(1, std::memory_order_release);
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
            accRisk.current_leverage.store(accRisk.current_exposure / accRisk.balance, std::memory_order_release);
        
    }

    // --------------------------------------CHECK FUNCTIONS-----------------------------------------------
    inline bool check_venue_halt_and_circuit(Order &order) noexcept
    {
        bool venue_halted = store_ram_.get_venue_flags(order.venue).halted.load(std::memory_order_acquire);
        if (UNLIKELY(venue_halted)) 
        {
                OrderWithRejectReason rej{order, static_cast<uint32_t>(RiskRejectReason::VenueTradingHalted)};
                risk_to_strategy_.push(rej);
                return true;
        }
        
        bool circuit_breaker = store_ram_.get_venue_flags(order.venue).circuit_breaker.load(std::memory_order_acquire);
        if (UNLIKELY(circuit_breaker))
        {
                OrderWithRejectReason rej{order, static_cast<uint32_t>(RiskRejectReason::CircuitBreakerTriggered)};
                risk_to_strategy_.push(rej);
                return true;
        }

        bool symbol_halted = store_ram_.get_symbolmeta(order.venue, order.instrument_id).halted.load(std::memory_order_acquire);
        if (UNLIKELY(symbol_halted))
        {
                OrderWithRejectReason rej{order, static_cast<uint32_t>(RiskRejectReason::SymbolTradingHalted)};
                risk_to_strategy_.push(rej);
                return true;
        }

        return false;
    }

    inline bool check_order_rate_limit(AccountRisk &accRisk, SymbolRisk &symRisk, Order &order, const uint8_t venue_index) noexcept
    {
        static constexpr double ns_per_cycle = 1 / 3e9; // 1 cycle ≈ 0.333 ns
        uint64_t now_ns = static_cast<uint64_t>(__rdtsc() * ns_per_cycle);

        uint32_t RejectReason = 0;
        auto &orderratelimit = accRisk.orderratelimit;
        bool max_orders_for_account_reached = false;
        bool max_orders_for_symbol_reached = false;

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
            RejectReason = static_cast<uint32_t>(RiskRejectReason::MaxOrderRateLimitExceededAccount);
            max_orders_for_account_reached = true;
        }

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
            RejectReason |= static_cast<uint32_t>(RiskRejectReason::MaxOrderRateLimitExceededSymbol);
            max_orders_for_symbol_reached = true;
        }

        if (RejectReason != 0) {
            OrderWithRejectReason rej{order, RejectReason};
            risk_to_strategy_.push(rej);
        }

        return max_orders_for_account_reached || max_orders_for_symbol_reached;
    }

    inline bool check_cancel_rate_limit(AccountRisk &accRisk, SymbolRisk &symRisk, Order &order, const uint8_t venue_index) noexcept
    {
        static constexpr double ns_per_cycle = 1 / 3e9; // 1 cycle ≈ 0.333 ns
        uint64_t now_ns = static_cast<uint64_t>(__rdtsc() * ns_per_cycle);

        uint32_t RejectReason = 0;
        auto &cancelratelimit = accRisk.cancelratelimit;
        bool max_cancels_for_account_reached = false;
        bool max_cancels_for_symbol_reached = false;

        if (now_ns - cancelratelimit.last_cancel_timestamp_ns > 1'000'000'000ULL)
        {
            cancelratelimit.cancel_count_window = 0;
            cancelratelimit.last_cancel_timestamp_ns = now_ns;
        }
        
        cancelratelimit.cancel_count_window++;
        
        if (cancelratelimit.cancel_count_window > cancelratelimit.max_cancels_per_sec)
        {
            RejectReason = static_cast<uint32_t>(RiskRejectReason::MaxCancelRateLimitExceededAccount);
            max_cancels_for_account_reached = true;
        }

        auto &sym_cancelratelimit = symRisk.cancelratelimit;
        
        if (now_ns - sym_cancelratelimit.last_cancel_timestamp_ns > 1'000'000'000ULL)
        {
            sym_cancelratelimit.cancel_count_window = 0;
            sym_cancelratelimit.last_cancel_timestamp_ns = now_ns;
        }

        sym_cancelratelimit.cancel_count_window++;

        if (sym_cancelratelimit.cancel_count_window > sym_cancelratelimit.max_cancels_per_sec)
        {
            RejectReason |= static_cast<uint32_t>(RiskRejectReason::MaxCancelRateLimitExceededSymbol);
            max_cancels_for_symbol_reached = true;
        }

        if (RejectReason != 0) {
            OrderWithRejectReason rej{order, RejectReason};
            risk_to_strategy_.push(rej);
        }

        return max_cancels_for_account_reached || max_cancels_for_symbol_reached;
    }

   /*  inline bool check_duplicate_order_id(const auto &order_ids, Order &order, const uint8_t venue_index) noexcept
    {
        auto it = order_ids[venue_index].find(order.client_order_id);

            if (it != order_ids[venue_index].end()) {
                OrderWithRejectReason rej{order, static_cast<uint32_t>(RiskRejectReason::DuplicateOrderID)};
                risk_to_strategy_.push(rej);
                return true;
            }

        return false;
    } */

    inline uint32_t check_quantity_bounds(uint32_t qty, uint32_t min_qty, uint32_t max_qty) noexcept
    {
        if (UNLIKELY(qty < min_qty))
            return static_cast<uint32_t>(RiskRejectReason::MinOrderSizeExceeded); // reuse code for small orders
        if (UNLIKELY(qty > max_qty))
            return static_cast<uint32_t>(RiskRejectReason::MaxOrderSizeExceeded);
        return 0;
    }
    
    inline uint32_t check_price_tick_valid(int64_t price_scaled, int64_t tick_size_scaled) noexcept
    {
        if (UNLIKELY(tick_size_scaled <= 0))
            return static_cast<uint32_t>(RiskRejectReason::RiskEngineInternalError);
        if (UNLIKELY((price_scaled % tick_size_scaled) != 0))
            return static_cast<uint32_t>(RiskRejectReason::PriceTickInvalid);

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

        if(it != orderrisks_[venue_index].end())
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

    inline uint32_t check_account_risk(AccountRisk &accRisk, const AccountLimit &accLim, const int64_t price_scaled, const uint32_t qty) noexcept
    {
        check_acc_status(accRisk.status.load(std::memory_order_acquire));

        int64_t estimated_notional = price_scaled * qty;
        int64_t estimated_margin_needed = estimated_notional / accRisk.current_leverage * 1'000'000ULL; // assuming leverage is scaled by 1,000,000    

        check_free_margin(accRisk.balance.load(std::memory_order_acquire), accRisk.total_unrealized_pnl.load(std::memory_order_acquire), estimated_margin_needed, accRisk.used_margin.load(std::memory_order_acquire));   
        check_balance(accRisk.balance.load(std::memory_order_acquire), estimated_margin_needed);
        check_max_leverage(accRisk.current_leverage.load(std::memory_order_acquire), accLim.max_leverage);
        check_max_notional(accRisk.current_exposure.load(std::memory_order_acquire), estimated_notional, accLim.max_notional);
        check_max_drawdown(accRisk.daily_realized_pnl.load(std::memory_order_acquire), accLim.max_daily_loss);

        return 0; // OK
    }
    
    inline uint32_t check_market_data_and_price_band_helper(int64_t best_bid_scaled, int64_t best_ask_scaled, int64_t price_scaled, int64_t max_price_deviation) noexcept
    {
        // Market data yoksa
        if (UNLIKELY(best_bid_scaled == 0 && best_ask_scaled == 0))
            return static_cast<uint32_t>(RiskRejectReason::MarketDataUnavailable);

        // Mid-price
        int64_t mid_price = (best_bid_scaled + best_ask_scaled) / 2;

        // Price deviation kontrolü
        if (UNLIKELY(max_price_deviation > 0 && std::llabs(price_scaled - mid_price) > max_price_deviation))
        {
            return static_cast<uint32_t>(RiskRejectReason::PriceOutsideMarketBand);
        }

        return 0; // OK
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
            // integer division ile ratio kontrolü
            int64_t ratio_check = price_scaled / mid_price;
            if (UNLIKELY(ratio_check > fat_finger_ratio_scaled))
                return static_cast<uint32_t>(RiskRejectReason::InvalidPriceRange);
        }

        return 0;
    }
 
};

