#include "HashTables.h"

HashTables::HashTables(const int venue_count) noexcept
{
   hash_tables.resize(venue_count);
}
