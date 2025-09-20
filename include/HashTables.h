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

    std::vector<std::vector<HashEntry>> hashtables_;
    std::vector<size_t> symbols_count_per_venue_;

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
        symbols_count_per_venue_.push_back(size);
        std::vector<HashEntry> hashtable(size);
       
        for (const auto &entry : symbol_table)
        {
            if (entry.symbol[0] == '\0')
                continue;

            uint32_t hash_val = hash_symbol(entry.symbol, size);

            while (hashtable[hash_val].valid)
            {
                hash_val = (hash_val + 1) & (size - 1);
            }

            strncpy(hashtable[hash_val].symbol, entry.symbol, 8);
            hashtable[hash_val].index = entry.index;
            hashtable[hash_val].valid = true;
        }

        hashtables_.push_back(hashtable);
    }

public:
    HashTables(const int venue_count) noexcept;

    template <size_t N>
    void initialize_all(const std::array<std::span<const SymbolIndex>, N> &all_symbol_tables) noexcept
    {
        for (const auto &symbol_table : all_symbol_tables)
            initialize(symbol_table);
    }

    inline uint32_t getIndex(size_t venue_index, const std::array<char, 8> &symbol) const noexcept
    {
        size_t size = hashtables_[venue_index].size();
        uint32_t hash_val = hash_symbol(symbol.data(), size);

        while (hashtables_[venue_index][hash_val].valid)
        {
            if (*reinterpret_cast<const uint64_t *>(hashtables_[venue_index][hash_val].symbol) ==
                *reinterpret_cast<const uint64_t *>(symbol.data()))
            {
                return hashtables_[venue_index][hash_val].index;
            }
            hash_val = (hash_val + 1) & (size - 1);
        }

        return UINT32_MAX;
    }

    inline const size_t get_symbol_count(const uint8_t venue_index) const noexcept
    {
        return symbols_count_per_venue_[venue_index];
    }
};