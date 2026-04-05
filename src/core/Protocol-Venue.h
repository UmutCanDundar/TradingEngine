// Common protocol & venue identifiers shared across the system.

#pragma once

#include <cstdint>
#include <cstddef>

enum class Protocol : uint8_t
{

   FIX,
   ITCH,
   OUCH,
   Unknown,
};

enum class Venue : uint8_t
{
   BIST,
   NASDAQ,
   Unknown,
};

inline constexpr size_t VENUE_COUNT = 2;
inline constexpr size_t PROTOCOL_COUNT = 3;