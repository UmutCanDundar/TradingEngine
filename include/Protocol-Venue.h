#pragma once

#include <cstdint>

enum class Protocol : uint8_t
{
   FIX,
   ITCH,
   SBE
};

enum class Venue : uint8_t
{
   BIST
};
