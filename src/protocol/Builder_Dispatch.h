// ========================================================================================================
// Builder_Dispatch
//
// PURPOSE:
// - Central dispatch stage responsible for routing validated Order objects
//   to the appropriate protocol- and venue-specific message builders.
// - Acts as the single entry point for transforming internal Order representations
//   into outbound wire-level protocol messages (FIX / OUCH).
//
// THREAD SAFETY:
// - Designed for single-threaded execution.
// - All internal queues are single-producer / single-consumer (SPSC).
// - No internal locking or synchronization is performed beyond required atomics
//   for cross-stage coordination.
//
// PERFORMANCE & DESIGN NOTES:
// - Dispatch is implemented using a fixed-size member function pointer lookup table
//   indexed by (Protocol, Venue).
// - Although switch-based dispatch with direct calls could theoretically allow
//   additional inlining opportunities, the current approach provides predictable
//   control flow and minimal instruction cache pressure.
// - This dispatch layer is intentionally kept thin:
//     - It orchestrates message flow and sequencing
//     - It does NOT perform heavy protocol encoding itself
// - All protocol-specific encoding logic resides inside dedicated builder
//   implementations (Builder_FIX, Builder_OUCH_*), where aggressive inlining,
//   allocation-free designs, and low-level optimizations are applied.
// - All builders except Builder_FIX(it must be owned by upper level compoonent because it
//   is given as reference to another component with the same level as Builder_Dispatch) are owned
//   directly as value members.
//   This design provides:
//     - Explicit and non-transferable ownership semantics
//     - Stable lifetimes matching the dispatch stage lifetime
//     - Improved cache locality by colocating builders within a single object
//     - Elimination of heap allocation, pointer indirection, and smart pointer overhead
// - Using smart pointers (e.g. std::unique_ptr) would not add meaningful semantic value,
//   as builder ownership is static and never transferred.
//
// DEVELOPER NOTES (PRE-PROFILING):
// - This dispatch layer exists to support a unified multi-protocol, multi-venue outbound pipeline.
// - If the system evolves toward dedicated per-protocol or per-venue builder threads, this
//   dispatch stage may be simplified or removed entirely.
// - Alternative dispatch strategies (e.g. switch-based direct calls) can be evaluated after
//   profiling if dispatch latency becomes measurable relative to protocol encoding cost.
// - At the current stage, optimizing this layer for marginal nanosecond gains would add unnecessary
//   complexity and reduce architectural clarity.
// - This class MUST NOT be instantiated on the stack due to the high memory occupation of all member
//   builders.
// ========================================================================================================

#pragma once

#include "Builder_OUCH_BIST.h"
#include "Builder_OUCH_NASDAQ.h"
#include "Order.h"
#include "FIXMessage.h"
#include "SoupBinTcp.h"
#include "NetworkPackets.h"
#include "Protocol-Venue.h"

#include <array>
#include <cstdint>

class Builder_FIX;
class LoginController;
class SessionManager;
class OrderManager;
class Parser_FIX;

class Builder_Dispatch
{
private:
  
    using BuilderFunc = void (Builder_Dispatch::*)(Order*) noexcept;
    std::array<std::array<BuilderFunc, VENUE_COUNT>, PROTOCOL_COUNT> makeBuilderLookUpTable() noexcept; 
    std::array<std::array<BuilderFunc, VENUE_COUNT>, PROTOCOL_COUNT> builder_table_;
    
    spscTxPacketQueue_t& builder_to_sender_;
    spscOrderQueue_t &risk_to_builder_;
    spscFIXOutSessionQueue_t& parser_to_fixbuilder_out_;
    spscFIXInSessionQueue_t &parser_to_fixbuilder_in_;
    SessionManager& sess_mngr_;
    SoupBinTcp& sbt_;
    LoginController& login_;
    TxPacketPoolManager &txPkt_pool_;
    Builder_FIX& fixBuilder_;
    OrderManager& ord_mngr_;
    Parser_FIX &fixParser_;

    Builder_OUCH_BIST ouchBuilder_bist_{sbt_, sess_mngr_};
    Builder_OUCH_NASDAQ ouchBuilder_nasdaq_{sbt_};

public:
    Builder_Dispatch(spscTxPacketQueue_t &builder_to_sender, spscOrderQueue_t &risk_to_builder, spscFIXOutSessionQueue_t &parser_to_fixbuilder_out, spscFIXInSessionQueue_t &parser_to_fixbuilder_in, 
                     SessionManager &sess_mngr, SoupBinTcp &sbt, LoginController &login, TxPacketPoolManager &txPkt_pool, Builder_FIX &fixBuilder, OrderManager &ord_mngr, Parser_FIX& fixParser) noexcept;

    bool dispatch() noexcept;

private:
    void buildFIX(Order *order) noexcept;
    void buildOUCH_BIST(Order *order) noexcept;
    void buildOUCH_NASDAQ(Order* order) noexcept;

    void buildFIX_ses_in(FIXSessionMessage &fixSesMsg, uint8_t session_index) noexcept;
    void buildFIX_ses_out(FIXSessionMessage &fixSesMsg, uint8_t session_index) noexcept;
    void buildFIX_app(Order* order, uint8_t session_index) noexcept;
   
};
