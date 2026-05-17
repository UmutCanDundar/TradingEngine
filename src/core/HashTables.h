// =============================================================================
// HashTables
//
// PURPOSE:
// - Maps symbols to indices per venue using open-addressing hash tables.
// - Detects duplicate exec_id values using a Bloom filter + fixed-size hash table.
//
// THREAD SAFETY:
// - Fully immutable after initialization.
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

class HashTables
{
private:
public:///
    struct HashEntry
    {
        char symbol[32] = {};
        uint32_t index;
        bool valid = false;
    };

    std::vector<std::vector<HashEntry>> hashtables_;
    std::vector<size_t> symbols_count_per_venue_;

    static constexpr size_t EXEC_ID_TABLE_SIZE = 1 << 4; // 262144 slots (~2.1 MB)
    static constexpr size_t MAX_PROBE = 8; 
    std::array<uint64_t, EXEC_ID_TABLE_SIZE> exec_id_hashes_;

    static constexpr size_t BLOOM_SIZE = 1 << 16; // 65536 poses (8 KB)
    std::array<uint64_t, BLOOM_SIZE / 64> bloom_filter_;

public:
    HashTables() noexcept;

    template <size_t N>
    void initialize_all(const std::array<std::span<const SymbolIndex>, N> &all_symbol_tables) noexcept
    {
        for (const auto &symbol_table : all_symbol_tables)
            initialize(symbol_table);
    }

    inline uint32_t getIndex(size_t venue_index, const std::array<char, 32> &symbol) const noexcept
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

    size_t get_symbol_count(const uint8_t venue_index) const noexcept;
    
    inline bool is_duplicate_exec_id(std::string_view exec_id) noexcept
    {
        const uint64_t hash = hash_exec_id(exec_id);

        // Phase 1: Bloom filter
        if (!bloom_contains(hash))
        {
            bloom_insert(hash);
            insert_exec_id_hash(hash);
            return false;
        }

        // Phase 2: Hash lookup 
        const size_t index = hash & (EXEC_ID_TABLE_SIZE-1);

        for (size_t i = 0; i < MAX_PROBE; ++i)
        {
            const size_t probed_index = (index + i) & (EXEC_ID_TABLE_SIZE - 1); //check interval(8) 
            const uint64_t stored_hash = exec_id_hashes_[probed_index];

            if (stored_hash == 0) 
            {
                exec_id_hashes_[probed_index] = hash;
                return false;
            }

            if (stored_hash == hash) 
                return true;
        }

        // Overwrite at original index 
        exec_id_hashes_[index] = hash;
        return false;
    }

private:
public: ///
    //-------------------Symbol Hashing----------------------
    void initialize(const std::span<const SymbolIndex> &symbol_table) noexcept;

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

    //-------------------Exec-Id Hashing----------------------
    static inline uint64_t hash_exec_id(std::string_view s) noexcept
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
            std::memcpy(&h2, p + (n - 8), 8); // tail 8B
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

    inline bool bloom_contains(uint64_t hash) const noexcept
    {
        const uint64_t pos1 = hash & (BLOOM_SIZE - 1);
        const uint64_t pos2 = (hash >> 16) & (BLOOM_SIZE - 1);

        const uint64_t word1 = bloom_filter_[pos1 / 64];
        const uint64_t word2 = bloom_filter_[pos2 / 64];

        const uint64_t bit1 = 1ULL << (pos1 % 64);
        const uint64_t bit2 = 1ULL << (pos2 % 64);

        return (word1 & bit1) && (word2 & bit2);
    }
    inline void bloom_insert(uint64_t hash) noexcept
    {
        const uint64_t pos1 = hash & (BLOOM_SIZE - 1);
        const uint64_t pos2 = (hash >> 16) & (BLOOM_SIZE - 1);

        bloom_filter_[pos1 / 64] |= (1ULL << (pos1 & 63));
        bloom_filter_[pos2 / 64] |= (1ULL << (pos2 & 63));
    }

    static inline uint64_t wyrot(uint64_t x, uint32_t k) noexcept
    {
        return (x << k) | (x >> (64 - k));
    }
    static uint64_t wymix(uint64_t a, uint64_t b) noexcept;
    static uint64_t wyhash_small(const char *p, size_t len, uint64_t seed) noexcept;
    

    inline void insert_exec_id_hash(uint64_t hash) noexcept
    {
        const size_t index = hash & (EXEC_ID_TABLE_SIZE-1);

        for (size_t i = 0; i < MAX_PROBE; ++i)
        {
            const size_t probed_index = (index + i) & (EXEC_ID_TABLE_SIZE-1); // open adressing
            if (exec_id_hashes_[probed_index] == 0)
            {
                exec_id_hashes_[probed_index] = hash;
                return;
            }
        }

        exec_id_hashes_[index] = hash;
    }
};