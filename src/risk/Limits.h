// ======================================================================================
// Limits
//
// PURPOSE:
// - Holds preloaded, read-only risk and trading limits for symbols and accounts.
// - Limits are loaded once at startup from configuration (JSON) and accessed
//   at runtime via constant-time lookups.
//
// THREAD SAFETY:
// - Thread-safe for concurrent read-only access after construction.
// - No atomics or locks are used.
//
// PERFORMANCE & DESIGN NOTES:
// - Cold-path initialization (JSON parsing, file I/O) is isolated in .cpp.
// - Runtime access consists only of array/vector indexing (O(1)).
// - Getter methods are inlined to avoid call overhead in latency-sensitive paths.
// - No manual alignment or padding is used to avoid cache and TLB pressure.
// - Memory layout favors dense packing over cache-line isolation, as data is
//   read-only and not subject to false sharing.
//
// DESIGN ASSUMPTIONS:
// - Venue ordering in configuration files must match internal venue indexing.
// - Symbol indices are resolved via HashTables during initialization.
//
// DEVELOPER NOTES (PRE-PROFILING):
// - No assumptions are made about hot/cold access patterns beyond read-only usage.
//
// - The current implementation assumes a single account per venue for limit
//   evaluation. While other parts of the system are designed with multi-account
//   support in mind, this class intentionally models one account per venue to
//   keep risk checks simple and fast. The structure can be extended to support
//   multiple accounts per venue if required in future engine revisions.
//
// - With VENUE_COUNT = 2, the Limits object has a raw size of ~184B, while symbol-level 
//   limit data is stored in heap-allocated vectors ~250 KB for typical BIST and NASDAQ 
//   symbol counts. The class is strictly read-only after construction and does not manage 
//   ownership of transient resources; its effective memory placement (stack or heap) is
//   determined by the owning engine component.
// =======================================================================================

#pragma once

#include "Protocol-Venue.h"

#include <cstdint>
#include <array>
#include <vector>

class HashTables;

struct SymbolLimit
{
    int64_t max_position_scaled; 
    int64_t max_order_notional_scaled; 
    int64_t max_price_deviation; 
    int64_t max_exposure_scaled;
    int64_t fat_finger_ratio; 
    uint32_t min_qty;       
    uint32_t max_qty;
    uint32_t price_scale_factor;
    uint32_t qty_scale_factor;
    uint32_t max_open_orders;
    uint32_t fat_finger_qty_threshold; 
};

struct AccountLimit
{
    int64_t max_notional; 
    int64_t max_daily_loss;
    int64_t max_unrealized_loss;
    int64_t maker_fee_rate; // in ppm (parts per million)
    int64_t taker_fee_rate; // in ppm (parts per million)
    int64_t max_leverage;
    uint32_t max_open_orders; 
};

class Limits
{
    private:
        HashTables &hashtables_;

        std::array<std::vector<SymbolLimit>,VENUE_COUNT> symbol_limits_;
        std::array<AccountLimit, VENUE_COUNT> account_limits_;

    public:
        Limits(HashTables &hashtables) noexcept;

        inline const SymbolLimit &getSymbolLimit(const int venue_index, const uint32_t symbol_index) const noexcept
        {
            return symbol_limits_[venue_index][symbol_index];
        }

        inline const AccountLimit &getAccountLimit(const int venue_index) const noexcept
        {
            return account_limits_[venue_index];
        }

    private:
        std::array<std::vector<SymbolLimit>, VENUE_COUNT> initialize_symbollimits() noexcept;
        std::array<AccountLimit, VENUE_COUNT> initialize_accountlimits() noexcept;

};

