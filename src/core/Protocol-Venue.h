// Common protocol & venue identifiers shared across the system.

#pragma once

#include <cstdint>
#include <cstddef>

enum class Protocol : uint8_t
{

   FIX = 0,
   ITCH = 1,
   OUCH = 2,
   Unknown = 3,
};

enum class Venue : uint8_t
{
   BIST = 0,
   NASDAQ = 1,
   Unknown = 2,
};

inline constexpr size_t VENUE_COUNT = 2;
inline constexpr size_t PROTOCOL_COUNT = 3;