// Generated automatically. DO NOT EDIT!

#pragma once

#include <array>
#include <cstdint>
#include <span>

struct SymbolIndex { const char* symbol; uint32_t index; };

constexpr std::array<SymbolIndex, 4> BIST_symbols = {{
    {"A", 0},
    {"B", 1},
    {"C", 2},
    {"", UINT32_MAX},
}};

constexpr std::array<SymbolIndex, 4> NASDAQ_symbols = {{
    {"AAPL", 0},
    {"MSFT", 1},
    {"GOOG", 2},
    {"", UINT32_MAX},
}};

inline auto all_symbol_tables = std::to_array<std::span<const SymbolIndex>>({
    std::span{BIST_symbols},
    std::span{NASDAQ_symbols},
});

