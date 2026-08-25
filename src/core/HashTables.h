// =============================================================================
// HashTables
//
// PURPOSE:
// - Maps symbols to indices per venue using open-addressing hash tables.
// - Detects duplicate exec_id values using a Bloom filter + fixed-size hash table.
//
// THREAD SAFETY:
// - Fully immutable after initialization except the parts related to exec_id validation.
// - Safe for concurrent read-only access without synchronization.
//
// PERFORMANCE & DESIGN NOTES:
// - Read-heavy, hot-path usage.
// - Designed for low-latency symbol and exec_id lookups.
//
// DEVELOPER NOTES (PRE-PROFILING):
// - Contains large fixed-size arrays (~2.1 MB) heap allocation is preferred.
// - Inline/static-inline decisions made conservatively to reduce I-cache pressure.
// - Revisit after instruction-cache profiling.
// =============================================================================

#pragma once

#include <cstddef>     
#include <cstdint>    
#include <array>      
#include <cstring>    
#include <span>       
#include <vector>      
#include <string_view> 

struct SymbolIndex;

struct HashEntry
{
    std::array<char,32> symbol{};
    uint32_t index;
    bool valid = false;
};

class HashTables
{
private:
public: /// Debugging and monitoring fields. Will be removed after profiling and debugging.

    static constexpr size_t EXEC_ID_TABLE_SIZE = 1 << 18; // 262144 slots (~2.1 MB)
    static constexpr size_t MAX_PROBE = 8; 
    static constexpr size_t BLOOM_SIZE = 1 << 16; // 65536 poses (8 KB)

    std::vector<std::vector<HashEntry>> hashtables_;
    std::vector<size_t> symbols_count_per_venue_;
    std::array<uint64_t, EXEC_ID_TABLE_SIZE> exec_id_hashes_;
    std::array<uint64_t, BLOOM_SIZE / 64> bloom_filter_;

public:
    HashTables();
    size_t get_symbol_count(uint8_t venue_index) const noexcept;

    template <size_t N>
    void initialize_all(const std::array<std::span<const SymbolIndex>, N> &all_symbol_tables) 
    {
        for (auto symbol_table : all_symbol_tables)
            initialize(symbol_table);
    }

    uint32_t getIndex(const size_t venue_index, const std::array<char, 32> &symbol) const noexcept
    {
        auto& hashtable = hashtables_[venue_index];
        const size_t size = hashtable.size();
        uint32_t hash_val = hash_symbol(symbol.data(), size);
        
        while (hashtable[hash_val].valid)
        {
            if (std::memcmp(hashtable[hash_val].symbol.data(), symbol.data(), 8) == 0) // Assuming the first 8 chars is enough to compare symbols.
            {
                return hashtable[hash_val].index;
            }
            hash_val = (hash_val + 1) & (size - 1);
        }
        return UINT32_MAX;
    }

    bool is_duplicate_exec_id(std::string_view exec_id) noexcept
    {
        const uint64_t hash = hash_exec_id(exec_id);

        if (!bloom_contains(hash))
        {
            bloom_insert(hash);
            insert_exec_id_hash(hash);
            return false;
        }

        const size_t index = hash & (EXEC_ID_TABLE_SIZE-1);

        for (size_t i = 0; i < MAX_PROBE; ++i)
        {
            const size_t probed_index = (index + i) & (EXEC_ID_TABLE_SIZE - 1); 
            const uint64_t stored_hash = exec_id_hashes_[probed_index];

            if (stored_hash == 0) 
            {
                exec_id_hashes_[probed_index] = hash;
                return false;
            }

            if (stored_hash == hash) 
                return true;
        }

        exec_id_hashes_[index] = hash;
        return false;
    }

private:
public: /// Debugging and monitoring fields. Will be removed after profiling and debugging.
    //-------------------Symbol Hashing----------------------
    void initialize(std::span<const SymbolIndex> symbol_table);

    bool bloom_contains(const uint64_t hash) const noexcept
    {
        const uint64_t pos1 = hash & (BLOOM_SIZE - 1);
        const uint64_t pos2 = (hash >> 16) & (BLOOM_SIZE - 1);

        const uint64_t word1 = bloom_filter_[pos1 / 64];
        const uint64_t word2 = bloom_filter_[pos2 / 64];

        const uint64_t bit1 = 1ULL << (pos1 % 64);
        const uint64_t bit2 = 1ULL << (pos2 % 64);

        return (word1 & bit1) && (word2 & bit2);
    }
    void bloom_insert(const uint64_t hash) noexcept
    {
        const uint64_t pos1 = hash & (BLOOM_SIZE - 1);
        const uint64_t pos2 = (hash >> 16) & (BLOOM_SIZE - 1);

        bloom_filter_[pos1 / 64] |= (1ULL << (pos1 & 63));
        bloom_filter_[pos2 / 64] |= (1ULL << (pos2 & 63));
    }

    void insert_exec_id_hash(const uint64_t hash) noexcept
    {
        const size_t index = hash & (EXEC_ID_TABLE_SIZE-1);

        for (size_t i = 0; i < MAX_PROBE; ++i)
        {
            const size_t probed_index = (index + i) & (EXEC_ID_TABLE_SIZE-1); 
            if (exec_id_hashes_[probed_index] == 0)
            {
                exec_id_hashes_[probed_index] = hash;
                return;
            }
        }

        exec_id_hashes_[index] = hash;
    }

    static uint64_t wymix(uint64_t a, uint64_t b) noexcept
    {
    a ^= wyrot(b, 17);
    b ^= wyrot(a, 33);
    a *= 0xff51afd7ed558ccdULL;
    b *= 0xc2b2ae3d27d4eb4fULL;
    a ^= a >> 29;
    b ^= b >> 31;
    return a ^ b;
    }

    static uint64_t wyhash_small(const char *p, size_t len, uint64_t seed) noexcept
    {
    uint64_t a = seed, b = seed ^ 0x9e3779b185ebca87ULL;
    uint64_t tmp = 0;
    std::memcpy(&tmp, p, len);
    return wymix(a ^ tmp, b ^ (uint64_t)len);
    }

    static uint64_t wyrot(uint64_t x, uint32_t k) noexcept
    {
        return (x << k) | (x >> (64 - k));
    }

    static uint32_t hash_symbol(const char *s, size_t size) noexcept
    {
        uint32_t h = 2166136261u;
        while (*s)
        {
            h ^= static_cast<uint8_t>(*s++);
            h *= 16777619u;
        }
        return h & (size - 1);
    }

    //-------------------Exec-Id Hashing----------------------
    static uint64_t hash_exec_id(std::string_view s) noexcept
    {
        const size_t n = s.size();
        const char *p = s.data();

        constexpr uint64_t SEED = 0x9e3779b185ebca87ULL;

        if (n < 8)
        {
            return (wyhash_small(p, n, SEED) | 1ULL);
        }

        uint64_t h1 = 0, h2 = 0, h3 = 0;
       
        std::memcpy(&h1, p, 8);
        if (n < 16)
        {
            std::memcpy(&h2, p + (n - 8), 8); 
            return (wymix(h1 ^ (uint64_t)n, h2) | 1ULL);
        }

       
        std::memcpy(&h2, p + 8, 8);
        if (n > 16)
        {
            std::memcpy(&h3, p + (n - 8), 8);
            uint64_t m = wymix(h1, h2);
            return (wymix(m, h3 ^ (uint64_t)n) | 1ULL);
        }

        // n = 16
        return (wymix(h1, h2 ^ (uint64_t)n) | 1ULL);
    }
};