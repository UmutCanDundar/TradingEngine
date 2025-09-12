// Generated automatically. DO NOT EDIT!

#pragma once

#include <unordered_map>
#include <cstdint>
#include "Protocol-Venue.h"

struct alignas(64) AccountLimits
{
    int64_t max_position = 0; // done
    int64_t max_daily_loss = 0;
    double max_leverage = 0;
    uint32_t max_open_orders = 0; // done
    uint8_t pad[36];
};

const std::unordered_map<Venue, std::pair<uint64_t, uint64_t>> VenueLimits{
    {Venue::BIST, {5, 1000000}},
    {Venue::NYSE, {5, 1000000}},
    {Venue::NASDAQ, {5, 1000000}},
};