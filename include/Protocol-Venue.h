#pragma once

#include <cstdint>
#include <cstddef>

enum class Protocol : uint8_t
{

   FIX,
   ITCH,
   OUCH,
   SBE,
   Unknown,
};

enum class Venue : uint8_t
{
   BIST,
   NYSE,
   NASDAQ,
   Unknown,
};

inline constexpr size_t VENUE_COUNT = 3;
inline constexpr size_t PROTOCOL_COUNT = 4;