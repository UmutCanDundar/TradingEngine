#pragma once

#include "Protocol-Venue.h"

#include <cstdint>
#include <array>
#include <vector>

class HashTables;

struct alignas(64) SymbolLimit
{
    int64_t max_position_scaled; 
    int64_t max_notional_scaled; 
    int64_t max_price_deviation; 
    int64_t fat_finger_ratio = 0; 
    uint32_t min_qty;       
    uint32_t max_qty;
    uint32_t price_scale_factor;
    uint32_t qty_scale_factor;
    uint32_t max_open_orders = 0;
    uint32_t fat_finger_qty_threshold = 0; 
   
    uint8_t pad[16]; 
};

struct alignas(64) AccountLimit
{
    int64_t max_notional; 
    int64_t max_position = 0; 
    int64_t max_daily_loss = 0;
    int64_t max_unrealized_loss = 0;
    int64_t maker_fee_rate = 0; // in ppm (parts per million)
    int64_t taker_fee_rate = 0; // in ppm (parts per million
    int64_t max_leverage = 0;
    uint32_t max_open_orders = 0; 
   
    uint8_t pad[20];
};

class Limits
{
    private:
        std::array<std::vector<SymbolLimit>,VENUE_COUNT> symbol_limits_;
        std::array<AccountLimit,VENUE_COUNT> account_limits_;

        HashTables &hashtables_;
    
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

