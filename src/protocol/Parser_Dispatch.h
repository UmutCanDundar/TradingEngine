// ========================================================================================================
// Parser_Dispatch
//
// PURPOSE:
// - Central dispatch stage that routes decoded network packets to the appropriate
//   protocol- and venue-specific parsers.
// - Owns all protocol parsers and acts as the single entry point for the parsing pipeline.
//
// THREAD SAFETY:
// - Designed for single-threaded execution.
// - All owned parsers and internal queues assume single-producer semantics.
// - No internal synchronization is performed.
//
// PERFORMANCE & DESIGN NOTES:
// - Dispatch is implemented via a fixed-size function pointer lookup table to avoid
//   deep branch trees and improve branch prediction stability.
// - Although switch-based dispatch with direct calls could theoretically allow
//   additional inlining opportunities, the current approach provides predictable
//   control flow and minimal instruction cache pressure.
// - Wrapper dispatch functions are intentionally not inlined, as they are invoked
//   through a member function pointer table and cannot be inlined by the compiler.
// - Actual parsing hot paths reside within protocol-specific parser implementations
//   (FIX / ITCH / OUCH), where aggressive inlining and allocation-free designs are applied.
// - All protocol parsers are owned directly as value members rather than via
//   pointers or smart pointers.
//   This design provides:
//     - Clear and explicit ownership semantics.
//     - Stable object lifetimes matching the dispatch stage lifetime.
//     - Improved cache locality by colocating parsers in a single contiguous memory region.
//     - Avoidance of additional pointer indirection and heap allocations, which could introduce 
//       unnecessary latency and cache/TLB pressure.
//   And Also:
//      - Using smart pointers (e.g. std::unique_ptr) would not add meaningful semantic value here, 
//        as parser ownership is non-transferable throughout the lifetime of the system.
//
// DEVELOPER NOTES (PRE-PROFILING):
// - This dispatch layer exists to support a multi-protocol, multi-venue pipeline.
//   If the system evolves toward dedicated pipelines per protocol / venue with
//   sufficient CPU resources, this dispatch stage may be removed entirely.
// - Alternative dispatch strategies (e.g. switch-based direct calls) can be evaluated
//   after profiling if dispatch latency becomes measurable relative to parsing cost. At the current stage,
//   optimizing the dispatch layer for marginal nanosecond gains would add unnecessary complexity.
// - This class MUST NOT be instantiated on the stack due to the high memory occupation of all member
//   parsers.
// ========================================================================================================

#pragma once

#include "MessageWithVenue.h"
#include "Protocol-Venue.h"
#include "DbTypes.h"
#include "NetworkPackets.h"

#include "Parser_FIX.h"
#include "Parser_ITCH_BIST.h"
#include "Parser_ITCH_NASDAQ.h"
#include "Parser_OUCH_BIST.h"
#include "Parser_OUCH_NASDAQ.h"

#include <variant>
#include <array>
#include <cstdint>
#include <cstddef>

#include <boost/lockfree/spsc_queue.hpp>

struct OutPacket;
struct Order;
class NetworkIO;
class SessionManager;

class Parser_Dispatch
{
//private:
public:
    static constexpr uint16_t DB_QUEUE_THRESHOLD = 256;
    static inline uint16_t PktCount = 1;

    using ParserFunc = void (Parser_Dispatch::*)(OutPacket*) noexcept;
    std::array<std::array<ParserFunc, VENUE_COUNT>, PROTOCOL_COUNT> makeParserLookUpTable() noexcept; 
    std::array<std::array<ParserFunc, VENUE_COUNT>, PROTOCOL_COUNT> parser_table_;

    spscOutPacketQueue_t &receiver_to_parser_;
    spscMessageQueue_t &parser_to_store_;
    spscFIXOutSessionQueue_t &parser_to_fixbuilder_out_;
    spscFIXInSessionQueue_t &parser_to_fixbuilder_in_;
    SessionManager &sess_mngr_;
    spscDbQueue_t &db_to_parser_;
    NetworkIO &network_io_;

    Parser_FIX fixparser_{parser_to_fixbuilder_in_};
    Parser_ITCH_BIST itchparser_bist_;
    Parser_ITCH_NASDAQ itchparser_nasdaq_;
    Parser_OUCH_BIST ouchparser_bist_;
    Parser_OUCH_NASDAQ ouchparser_nasdaq_;

public:
    Parser_Dispatch(spscOutPacketQueue_t &receiver_to_parser, spscMessageQueue_t &parser_to_store, spscFIXOutSessionQueue_t &parser_to_fixbuilder_out, spscFIXInSessionQueue_t &parser_to_fixbuilder_in, 
                    SessionManager &sess_mngr, spscDbQueue_t &db_to_parser, NetworkIO &network_io) noexcept;

    bool dispatch() noexcept;
   
//private: 
public:
    void flush_DbQueue() noexcept;
    void proceedPendingFIX(auto* variant_msg, auto& seq_fix, SessionState& state) noexcept;
    void parseFIX(OutPacket* pkt) noexcept;
    void parseITCH_BIST(OutPacket *pkt) noexcept;
    void parseITCH_NASDAQ(OutPacket *pkt) noexcept;
    void parseOUCH_BIST(OutPacket *pkt) noexcept;
    void parseOUCH_NASDAQ(OutPacket *pkt) noexcept;
 
};
