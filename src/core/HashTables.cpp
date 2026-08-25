#include "HashTables.h"
#include "GeneratedSymbolTables.h"
#include "Protocol-Venue.h"

#include <algorithm>

HashTables::HashTables() 
{
   symbols_count_per_venue_.reserve(VENUE_COUNT);
   initialize_all(all_symbol_tables);
}

size_t HashTables::get_symbol_count(const uint8_t venue_index) const noexcept
{
   return symbols_count_per_venue_[venue_index];
}

void HashTables::initialize(std::span<const SymbolIndex> symbol_table) 
{
   const size_t size = symbol_table.size();
   symbols_count_per_venue_.push_back(size);
   std::vector<HashEntry> hashtable(size);

   for (auto sym_idx : symbol_table)
   {
      if (!sym_idx.symbol)
         continue;

      uint32_t hash_val = hash_symbol(sym_idx.symbol, size);

      while (hashtable[hash_val].valid) // 1 non-valid slot guaranteed in config
      {
         hash_val = (hash_val + 1) & (size - 1);
      }

      const size_t len = strnlen(sym_idx.symbol, 31);
      std::memcpy(hashtable[hash_val].symbol.data(), sym_idx.symbol, len+1);
      hashtable[hash_val].index = sym_idx.index;
      hashtable[hash_val].valid = true;
   }

   hashtables_.push_back(std::move(hashtable));
}

