// ============================================================================================================
// Sequence_SBT
//
// PURPOSE:
// - Maintains SoupBinTCP (SBT) session sequencing state, including last seen sequence number.
// - Provides fast accessors for sequence validation and increment on every SBT message.
// - Stores minimal session metadata (session ID, heartbeat interval) for bookkeeping.
//
// THREAD SAFETY:
// - Designed for single-thread ownership per SBT session.
// - No internal atomics or locks are used; correctness relies on strict session-thread binding.
//
// PERFORMANCE & DESIGN NOTES:
// - This class sits on the hot path: sequence increment and retrieval are invoked on every SBT message.
// - All accessors and mutators are intentionally inlined for minimal call overhead.
// - Session metadata is stored locally for cache locality and simplicity.
// - The class is designed to be instantiated per SBT session. Even if a venue uses only a single account,
//   the architecture supports multiple independent SBT sessions without shared sequencing state.
//
// DEVELOPER NOTES (PRE-PROFILING):
// - Memory footprint is small (~32 bytes per session). Lifetime is tied to the owning SessionManager, so no 
//   additional management is needed.
// - Conversion functions are simple but may be optimized if profiling shows string conversions are a bottleneck.
// ==============================================================================================================

#pragma once

#include <cstdint>
#include <cstring>
#include <cstddef>

struct SBTSequence
{
    uint64_t last_seen_seqnum{0};
    uint32_t heartbeat_interval{0};
    char session[10] = {};
};

class Sequence_SBT
{
private:
    SBTSequence sequence_;
  
public:
    inline void set_seq(const char* seqnum) noexcept { sequence_.last_seen_seqnum = strnum_to_int(seqnum); }
    inline void set_sess_id(const char *session) noexcept { std::memcpy(sequence_.session, session, 10); }

    inline void increase_seq() noexcept { sequence_.last_seen_seqnum++; }

    inline char* get_sess_id() noexcept { return sequence_.session; }
    inline char* get_seq() noexcept { return int_to_strnum(sequence_.last_seen_seqnum); }

private : 
    static inline uint64_t strnum_to_int(const char *strnum) noexcept
    {
        uint64_t result = 0;
        for (size_t i = 0; i < 20; ++i)
        {
            auto c = strnum[i];
            if (c >= '0' && c <= '9')
                result = result * 10 + (strnum[i] - '0');
        }
        return result;
    }

    static inline char* int_to_strnum(uint64_t value) noexcept
    {
        static char buffer[21]; 
        char *p = buffer;

        if (value == 0)
        {
            *p++ = '0';
            return p;
        }

        char *start = p;
        while (value > 0)
        {
            *p++ = static_cast<char>('0' + (value % 10));
            value /= 10;
        }

        for (char *first = start, *last = p - 1; first < last; ++first, --last)        
        {
            char tmp = *first;
            *first = *last;
            *last = tmp;
        }

        return p;
    }
    
};

    