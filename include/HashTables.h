#pragma once

#include "GeneratedSymbolTables.h"

#include <cstddef>
#include <cstdint>
#include <array>
#include <cstring>
#include <span>
#include <vector>

class HashTables
{
private:
    struct HashEntry
    {
        char symbol[8] = {};
        uint32_t index;
        bool valid = false;
    };

    std::vector<std::vector<HashEntry>> hash_tables;

    static inline uint32_t hash_symbol(const char *s, size_t size) noexcept
    {
        uint32_t h = 2166136261u;
        while (*s)
        {
            h ^= static_cast<uint8_t>(*s++);
            h *= 16777619u;
        }
        return h & (size - 1);
    }

    void initialize(const std::span<const SymbolIndex> &symbol_table) noexcept
    {
        size_t size = symbol_table.size();
        std::vector<HashEntry> hash_table(size);

        for (const auto &entry : symbol_table)
        {
            if (entry.symbol[0] == '\0')
                continue;

            uint32_t hash_val = hash_symbol(entry.symbol, size);

            while (hash_table[hash_val].valid)
            {
                hash_val = (hash_val + 1) & (size - 1);
            }

            strncpy(hash_table[hash_val].symbol, entry.symbol, 8);
            hash_table[hash_val].index = entry.index;
            hash_table[hash_val].valid = true;
        }

        hash_tables.push_back(hash_table);
    }

public:
    HashTables(const int venue_count) noexcept;

    template <size_t N>
    void initialize_all(const std::array<std::span<const SymbolIndex>, N> &all_symbol_tables) noexcept
    {
        for (const auto &symbol_table : all_symbol_tables)
            initialize(symbol_table);
    }

    inline uint32_t getIndex(size_t hashtable_index, const std::array<char, 8> &symbol) const noexcept
    {
        size_t size = hash_tables[hashtable_index].size();
        uint32_t hash_val = hash_symbol(symbol.data(), size);

        while (hash_tables[hashtable_index][hash_val].valid)
        {
            if (*reinterpret_cast<const uint64_t *>(hash_tables[hashtable_index][hash_val].symbol) ==
                *reinterpret_cast<const uint64_t *>(symbol.data()))
            {
                return hash_tables[hashtable_index][hash_val].index;
            }
            hash_val = (hash_val + 1) & (size - 1);
        }

        return UINT32_MAX;
    }
};