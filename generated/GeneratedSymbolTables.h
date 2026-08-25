// Generated automatically. DO NOT EDIT!

#pragma once

#include <array>
#include <cstdint>
#include <span>

struct SymbolIndex { const char* symbol{nullptr}; uint32_t index; }; // 12B > safe to copy

inline constexpr std::array<const SymbolIndex, 1> BIST_symbols = {{
    {"GARAN", 0},
}};

inline constexpr std::array<const SymbolIndex, 1> NASDAQ_symbols = {{
    {"AAPL", 0},
}};

inline constexpr auto all_symbol_tables = std::to_array<std::span<const SymbolIndex>>({
    std::span{BIST_symbols},
    std::span{NASDAQ_symbols},
});

