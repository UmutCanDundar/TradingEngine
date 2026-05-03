// Common protocol & venue identifiers shared across the system.

#pragma once

#include <cstdint>
#include <cstddef>

enum class Protocol : int
{

   FIX = 0,
   ITCH = 1,
   OUCH = 2,
   Unknown = 3,
};

enum class Venue : int
{
   BIST = 0,
   NASDAQ = 1,
   Unknown = 2,
};

inline constexpr size_t VENUE_COUNT = 2;
inline constexpr size_t PROTOCOL_COUNT = 3;