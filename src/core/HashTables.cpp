#include "HashTables.h"
#include "GeneratedSymbolTables.h"

HashTables::HashTables() noexcept
{
   initialize_all(all_symbol_tables);
}

size_t HashTables::get_symbol_count(const uint8_t venue_index) const noexcept
{
   return symbols_count_per_venue_[venue_index];
}

void HashTables::initialize(const std::span<const SymbolIndex> &symbol_table) noexcept
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

      strncpy(hashtable[hash_val].symbol, entry.symbol, 32);
      hashtable[hash_val].symbol[32 - 1] = '\0';
      hashtable[hash_val].index = entry.index;
      hashtable[hash_val].valid = true;
   }

   hashtables_.push_back(hashtable);
}

uint64_t HashTables::wymix(uint64_t a, uint64_t b) noexcept
{
   a ^= wyrot(b, 17);
   b ^= wyrot(a, 33);
   a *= 0xff51afd7ed558ccdULL;
   b *= 0xc2b2ae3d27d4eb4fULL;
   a ^= a >> 29;
   b ^= b >> 31;
   return a ^ b;
}

uint64_t HashTables::wyhash_small(const char *p, size_t len, uint64_t seed) noexcept
{
   uint64_t a = seed, b = seed ^ 0x9e3779b185ebca87ULL;
   uint64_t tmp = 0;
   std::memcpy(&tmp, p, len);
   return wymix(a ^ tmp, b ^ (uint64_t)len);
}