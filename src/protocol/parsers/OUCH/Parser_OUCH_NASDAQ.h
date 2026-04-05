// ======================================================================================================
// Parser_OUCH_NASDAQ
//
// PURPOSE:
// - High-performance, allocation-free parser for NASDAQ OUCH messages.
// - Decodes raw OUCH byte streams into strongly-typed message objects.
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

#include "OUCHTypes_NASDAQ.h"
#include "MessagePool.h"

#include <cstddef>
#include <array>
#include <variant>
#include <type_traits>

class Parser_OUCH_NASDAQ
{
private:
    static constexpr size_t MAX_MESSAGES = 14;

    MessagePools<NASDAQ::OUCHMessageTypes> ouch_pools_;

    using MessageHandlerFunc = void (*)(Parser_OUCH_NASDAQ&, const char *, NASDAQ::OUCHMessage &) noexcept;
    static const std::array<MessageHandlerFunc, MAX_MESSAGES> makeMessageHandlersLookup() noexcept;
    static const std::array<MessageHandlerFunc, MAX_MESSAGES> MessageHandlers;

public:
    Parser_OUCH_NASDAQ() noexcept;

    template <typename T>
    inline void release(T *ouchMsg)
    {
        ouch_pools_.get_pool<T>().release(ouchMsg);
    }

    inline void releaseOUCH(NASDAQ::OUCHMessage &ouchMsg)
    {
        std::visit([this](auto *ptr)
                   {
                       using T = std::remove_pointer_t<decltype(ptr)>;
                       release<T>(ptr); },
                   ouchMsg);
    }

    inline NASDAQ::OUCHMessage parse(const char *data) noexcept
    {
        NASDAQ::OUCHMessage OUCHmsg;
        size_t index = static_cast<size_t>(NASDAQ::ouchMessageIndex(*data));
        if (index != 99)
            MessageHandlers[index](*this, data, OUCHmsg);
        return OUCHmsg;
    }
};
