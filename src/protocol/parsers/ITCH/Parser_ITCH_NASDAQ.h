// ======================================================================================================
// Parser_ITCH_NASDAQ
//
// PURPOSE:
// - High-performance, allocation-free parser for NASDAQ ITCH market data messages.
// - Decodes raw ITCH byte streams into strongly-typed message objects.
//
// THREAD SAFETY:
// - The parser is designed for single-threaded usage.
// - Since messagePools allocate message objects that become effectively read-only
//   after being filled by the parser, no additional alignment or false-sharing mitigation is required.

//
// PERFORMANCE & DESIGN NOTES:
// - A function pointer table is preferred over switch-case dispatch to avoid
//   deep branch trees and improve instruction cache predictability.
// - Template-based dispatch or specialization was intentionally avoided since
//   all message handlers share identical runtime signatures and template
//   instantiations would only increase code size without providing measurable
//   latency benefits.
// - Message objects are allocated from fixed-capacity, type-specific pools.
//
// DEVELOPER NOTES (PRE-PROFILING):
// - Although the current system operates with a single pipeline and a single account per venue,
//   the message pools are intentionally designed as parser-owned members and handler functions
//   receive the parser instance by reference. This design choice anticipates potential future
//   extensions where multiple accounts and multiple independent pipelines per venue may coexist.
//   The decision is made consciously to keep the system future-proof, and it is expected that
//   passing the parser reference into handlers does not introduce any measurable latency impact
//   in the current single-account-per-venue execution model.
//
// - This class is currently owned by Parser_Dispatch, and its memory location depends on that owner.
//   If this class is seperated in the future:
//    * Raw object size is ~180 KB, however allocation decisions for this class are NOT based on size.
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
//  - Parser handlers are intentionally dispatched via a function table. Since this path is
//    memory-bound (dominated by memcpy/cache behavior), so inline or constexpr dispatch was not expected
//    to provide meaningful latency improvement upfront and inlining all handlers would cause code bloat
//    and instruction cache pressure. After profiling, frequently-hit (hot) handlers may be separated and
//    inlined via switch-case or template dispatch, while cold handlers remain in the function table.
// =====================================================================================================

#pragma once

#include "ITCHTypes_NASDAQ.h"
#include "MessagePool.h"
#include "NetworkPackets.h"

#include <cstddef>
#include <array>
#include <variant>
#include <type_traits>

class Parser_ITCH_NASDAQ
{
private:
public: ///
    static constexpr size_t MAX_MESSAGES = 11;

    MessagePools<NASDAQ::ITCHMessageTypes> itch_pools_;

    using MessageHandlerFunc = void (*)(Parser_ITCH_NASDAQ& ,const char*, NASDAQ::ITCHMessage&) noexcept;
    static const std::array<MessageHandlerFunc, MAX_MESSAGES> makeMessageHandlersLookup() noexcept;
    static const std::array<MessageHandlerFunc, MAX_MESSAGES> MessageHandlers;

public:
    Parser_ITCH_NASDAQ() noexcept;

    template <typename T>
    inline void release(T *itchMsg)
    {
        itch_pools_.get_pool<T>().release(itchMsg);
    }

    inline void releaseITCH(NASDAQ::ITCHMessage itchMsg)
    {
        std::visit([this](auto *ptr)
                    {
                    using T = std::remove_pointer_t<decltype(ptr)>;
                    release<T>(ptr); },
                    itchMsg);
    }

    inline NASDAQ::ITCHMessage parse(const char *data) noexcept
    {
        NASDAQ::ITCHMessage ITCHmsg;
        size_t index = NASDAQ::itchMessageIndex(*data);
        if (index != 99)
            MessageHandlers[index](*this, data, ITCHmsg);
        return ITCHmsg;
    }

    inline void ItchPacketHandler(RxPacket *pkt) noexcept 
    {
        size_t offset = 0;
        auto* data = pkt->data.data();

        while(offset < pkt->len)
        {
            size_t msg_size = NASDAQ::ITCHSizeOfs[NASDAQ::itchMessageIndex(*(data+offset))];
            pkt->offsets[pkt->msg_count++] = offset;
            offset += msg_size;
        }
    }
};
