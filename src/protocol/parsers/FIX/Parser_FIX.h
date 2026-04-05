// ===================================================================================================
// Parser_FIX
//
// PURPOSE:
// - Parses FIX messages (both application and session messages) from incoming data streams.
// - Handles session messages, and communicate to Builder_FIX via spsc queue if a message requiring
//   instant respond(e.g. TestReq, ResendReq,..) is received.
//
// THREAD SAFETY:
// - Single-threaded hot-path assumed (parser thread).
// - No internal synchronization for message pools; external queues handle inter-thread communication.
//
// PERFORMANCE & DESIGN NOTES:
// - Hot-path component; uses AVX2 SIMD for tag/value boundary detection.
// - Preallocated FIXMessage and FIXSessionMessage pools to avoid dynamic allocation.
// - Uses fixed-size arrays for handler lookup (O(1)) to minimize branch mispredictions.
// - Pending messages tracked via btree_map for eventual store.
//
// DEVELOPER NOTES (PRE-PROFILING):
// - Assuming that the incoming messages strictly conform to the FIX protocol and no bugs occur
//   on the exchange side, if any deviation from this assumption is observed during real-production
//   rehearsal, additional validation mechanisms will be introduced, including checksum verification
//   against received bytes and validation of critical tag values.
//
// - This class is currently owned by Parser_Dispatch, and its memory location depends on that owner.
//   If this class is seperated in the future:
//    * Raw object size is ~330 KB, however allocation decisions for this class are NOT based on size.
//    * According to the current architecture, ownership is expected to remain at a higher-level
//      controller (e.g. TradingEngine, heap-allocated) and accessed by a dedicated worker thread,
//      making stack allocation incompatible with the ownership and lifetime model.
//    * Even if the ownership model were changed (e.g. made thread-local or per-session), this would
//      still be a long-lived, stateful core component whose lifetime is tied to the owning context
//      rather than a single stack frame.
//    * Additionally, this component is expected to evolve and potentially grow in size as protocol
//      handling and internal state expand over time.
//      Therefore, such core components MUST be heap-allocated regardless of their raw size.
//
//  - Tag handlers are intentionally dispatched via a function table. Since this path is
//    memory-bound (dominated by sequential byte scanning and memory accesses), so inline or constexpr 
//    dispatch was not expected to provide meaningful latency improvement upfront and inlining all 
//    handlers would cause code bloat and instruction cache pressure. After profiling, frequently-hit 
//    (hot) tags may be separated and inlined via switch-case or template dispatch, while cold tags remain 
//    in the function table.
// =====================================================================================================

#pragma once

#include "common.h"
#include "FIXMessage.h"

#include <string_view>
#include <cstdint>
#include <array>
#include <cstddef>
#include <immintrin.h>
#include <variant>
#include <type_traits>
#include <algorithm>
#include <utility>
#include <map>

#include <absl/container/btree_map.h>
#include <boost/lockfree/spsc_queue.hpp>

enum class BodyLenState : uint8_t
{
    EXPECT_SOH,
    EXPECT_9,  
    EXPECT_EQ, 
    READ_VALUE, 
    DONE,
    None,
};

struct PartialFIXBuffer 
{
    static constexpr size_t DATA_SIZE = 1024;
    char data[DATA_SIZE];
    size_t existing_len = 0; // Partial FIX Buffer data offset
    size_t remaining_len = 0; // Remaining length of the FIX message to be received
    size_t body_len = 0;    
    size_t pkt_data_offset = 0; // Copied data from the packet, parser will process again the same packet from this offset until the all data is consumed.
    bool active = false;
    BodyLenState bodylen_state = BodyLenState::None;
};
    
struct OutPacket;
struct SessionState;
class Sequence_FIX;

class Parser_FIX
{
private:
    static constexpr char SOH = '\x01';
    static constexpr size_t MAX_TAG = 512;
    static constexpr size_t MAX_SESTAG = 2048;
    static constexpr size_t FIX_POOL_CAPACITY = 1024;

    using FIXMessagePool_t = std::array<FIXMessage, FIX_POOL_CAPACITY>;
    using spscFIXQueue_t = boost::lockfree::spsc_queue<FIXMessage *, boost::lockfree::capacity<FIX_POOL_CAPACITY>>;

    using FIXSesMessagePool_t = std::array<FIXSessionMessage, FIX_POOL_CAPACITY>;
    using spscFIXSesQueue_t = boost::lockfree::spsc_queue<FIXSessionMessage *, boost::lockfree::capacity<FIX_POOL_CAPACITY>>;
    
    using FIXPendingMap_t = absl::btree_map<uint32_t, std::variant<FIXMessage *, FIXSessionMessage *>>;
    std::map<uint32_t, std::variant<FIXMessage *, FIXSessionMessage *>> pending_to_store_map_;
    // FIXPendingMap_t pending_to_store_map_;
    spscFIXQueue_t free_fixMsg_list_;
    spscFIXSesQueue_t free_fixSesMsg_list_;
    
    FIXMessagePool_t fixMsg_pool_;
    FIXSesMessagePool_t fixSesMsg_pool_;

    PartialFIXBuffer partial_fix_msg_;
   
    using TagHandlerFunc = void (*)(std::string_view, FIXMessage *) noexcept;
    static const std::array<TagHandlerFunc, MAX_TAG> makeTagHandlersLookup() noexcept;
    static const std::array<TagHandlerFunc, MAX_TAG> tagHandlers;

    using SesTagHandlerFunc = void (*)(std::string_view, FIXSessionMessage *) noexcept;
    static const std::array<SesTagHandlerFunc, MAX_SESTAG> makeSesTagHandlersLookup() noexcept;
    static const std::array<SesTagHandlerFunc, MAX_SESTAG> SestagHandlers;

    spscFIXInSessionQueue_t &parser_to_fixbuilder_in_;

public:
    Parser_FIX(spscFIXInSessionQueue_t &parser_to_fixbuilder_in) noexcept;
    
    bool handle_sesMsg(FIXSessionMessage *fixSesMsg, Sequence_FIX &seq_fix, SessionState &state) noexcept;
    char find_type(const char *data) noexcept;
    void resend_logic(uint32_t msg_seqnum, Sequence_FIX &seq_fix) noexcept;
    void resend_logic_logon(FIXSessionMessage *fixSesMsg, Sequence_FIX &seq_fix) noexcept;
    std::pair<char *, size_t> nextFixMsg(OutPacket *pkt, size_t &data_offset) noexcept;

    template <typename T>
    T* parse(const char* data, size_t len) noexcept
    {
        T* msg{nullptr};

        if constexpr (std::is_same_v<T, FIXMessage>)
            free_fixMsg_list_.pop(msg);
        else
            free_fixSesMsg_list_.pop(msg);

/* //-----------------------------------DEBUG_ONLY-----------------------------------------------
IF_NOT_RELEASE (
   if(msg == nullptr) 
        return nullptr;
);
//-------------------------------------------------------------------------------------------- */
       
        auto const &handlers = []() -> auto const &
        {
            if constexpr (std::is_same_v<T, FIXMessage>)
                return tagHandlers;
            else
                return SestagHandlers;
        }();

        size_t pos = 0;
        size_t tag_start = 0;

        const __m256i eq_mask = _mm256_set1_epi8('=');
        const __m256i soh_mask = _mm256_set1_epi8('\x01');

        while (pos + 32 <= len)
        {
            __m256i chunk = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(data + pos));

            uint32_t eq_bits = _mm256_movemask_epi8(_mm256_cmpeq_epi8(chunk, eq_mask));
            uint32_t soh_bits = _mm256_movemask_epi8(_mm256_cmpeq_epi8(chunk, soh_mask));

            while (eq_bits != 0)
            {

                int eq_offset = __builtin_ctz(eq_bits);
                size_t eq_pos = pos + eq_offset;

                int soh_offset = 0;
                size_t soh_pos = static_cast<size_t>(-1);

                uint32_t soh_after = soh_bits & (~0u << (eq_offset + 1));

                if (soh_after != 0)
                {
                    soh_offset = __builtin_ctz(soh_after);
                    soh_pos = pos + soh_offset;
                }
                else
                {
                    size_t search_pos = eq_pos + 1;
                    while (search_pos < len && data[search_pos] != '\x01')
                        ++search_pos;
                    soh_pos = search_pos;
                }

                std::string_view value(data + eq_pos + 1, soh_pos - (eq_pos + 1));

                size_t tag = parseNum(std::string_view(data + tag_start, eq_pos - tag_start));

                auto& handler = handlers[tag];
                if (handler)
                    handler(value, msg);

                tag_start = soh_pos + 1;

                eq_bits &= ~(1u << eq_offset);
            }

            pos += 32;
        }

        while (pos < len)
        {
            size_t eq_pos = pos;
            while (data[eq_pos] != '=' && eq_pos < len)
                ++eq_pos;

            size_t tag = parseNum(std::string_view(data + tag_start, eq_pos - tag_start));
            size_t soh_pos = eq_pos + 1;
            
            if(UNLIKELY(eq_pos == len)) // if pos is between last eq and last soh("10=xxx\x01"), eq_pos will be out of bound, so we need to handle this case separately
            {
                soh_pos = eq_pos - 1;
                eq_pos -= 5;
                tag_start = eq_pos - 2;
                tag = parseNum(std::string_view(data + tag_start, eq_pos - tag_start));
            }
            
            while (data[soh_pos] != '\x01')
                ++soh_pos;

            std::string_view value(data + eq_pos + 1, soh_pos - (eq_pos + 1));

             auto& handler = handlers[tag];
             if (handler)
                handler(value, msg);

            tag_start = soh_pos + 1;
            pos = soh_pos + 1;
        }
        return msg;
    }

    inline void releaseFIX(FIXMessage* fixMsg) noexcept
    {
        free_fixMsg_list_.push(fixMsg);
    }
    inline void releaseFIX(FIXSessionMessage* fixSesMsg) noexcept
    {
        free_fixSesMsg_list_.push(fixSesMsg);
    }

    template<typename T>
    inline void push_pending(T* msg) noexcept
    {
        using type_t = std::conditional_t<std::is_same_v<T, FIXMessage>, FIXMessage*, FIXSessionMessage*>;

        pending_to_store_map_.emplace(
            msg->seqnum,
            std::variant<FIXMessage *, FIXSessionMessage *>{std::in_place_type<type_t>, msg});
    }

    inline std::variant<FIXMessage*, FIXSessionMessage*>* find_in_pending(const uint32_t expected) noexcept
    {
        auto it = pending_to_store_map_.find(expected);
        if (it == pending_to_store_map_.end())
            return nullptr;
        
        return &it->second;
    }

    inline void pop_pending(const auto seqnum) noexcept
    {
       pending_to_store_map_.erase(seqnum);
    }

private:

    static inline uint64_t parseNum(std::string_view strnum) noexcept
    {
        uint64_t result = 0;
        for (char c : strnum)
        {
            if (LIKELY(c >= '0' && c <= '9'))
            {
                result = result * 10 + (c - '0');
            }
        }
        return result;
    }

    static inline uint8_t parseChar(std::string_view str) noexcept
    {
        return static_cast<uint8_t>(str[0]);
    }

};


