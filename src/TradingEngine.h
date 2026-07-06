// ======================================================================================================
// TradingEngine
//
// PURPOSE:
// - Central orchestrator of the trading system, managing core components, SPSC queues,
//   flow threads, and high-level coordination between market data ingestion, order
//   processing, risk evaluation, and message building/sending pipelines.
//
// - This class is **responsible for creating and wiring all components**, including:
//     - Core structures (HashTables, MarketBook, Limits, SessionManager, LoginController, etc.)
//     - Queues for inter-thread communication
//     - Components that perform domain-specific logic (NetworkIO, Parser_Dispatch, OrderManager,
//       ClickhouseWriter(DB), RiskEngine, Builder_Dispatch)
//     - Flows that encapsulate threads and continuous processing loops (NetworkFlow, ParserFlow,
//       OrderFlow, RiskFlow, BuilderFlow)
//
// THREAD SAFETY:
// - Each flow runs in its own thread and operates on its own thread-local or queue-accessed state.
// - TradingEngine itself is **single-threaded**; all members are created and destroyed from the main
//   thread.
// - Inter-thread communication happens strictly via SPSC queues; no internal members are concurrently
//   mutated.
// - Atomic `running_` flag is used for safe start/stop coordination across flow threads.
//
// PERFORMANCE & DESIGN NOTES:
// - All components are **direct members of TradingEngine**; their memory allocation depends
//   on where TradingEngine itself is allocated (stack if local, heap if created via new/make_unique).
//   This layout ensures predictable relative memory addresses for components and maintains cache locality.
// - The order of member initialization ensures that dependencies (e.g., session_manager_ →
//   builder_fix_ → login_) are correctly constructed before use in flows.
//
// DEVELOPER NOTES (PRE-PROFILING):
// - This class **must be heap-allocated** in production environments due to large total size
//   (all member components, queues, and buffers), otherwise main thread stack overflow is likely.
// - Flows encapsulate threading, affinity, and real-time priority setup; TradingEngine orchestrates
//   start/stop.
// - Call `start()` to launch all flows and threads, `stop()` to terminate them safely.
// ======================================================================================================
#pragma once

#include <atomic>

#include "Order.h"
#include "FIXMessage.h"
#include "NetworkPackets.h"

// ===== CORE =====
#include "HashTables.h"
#include "MarketBook.h"
#include "Limits.h"
#include "SessionManager.h"
#include "LoginController.h"
#include "SoupBinTcp.h"

// ===== COMPONENTS =====
#include "NetworkIO.h"
#include "Parser_Dispatch.h"
#include "OrderManager.h"
#include "ClickhouseWriter.h"
#include "RiskEngine.h"
#include "Builder_Dispatch.h"
#include "Builder_FIX.h"
#include "Parser_FIX.h"

// ===== FLOWS =====
#include "NetworkFlow.h"
#include "ParserFlow.h"
#include "OrderFlow.h"
#include "RiskFlow.h"
#include "BuilderFlow.h"

class Builder_FIX;

class TradingEngine
{
public:
    TradingEngine() noexcept;
    ~TradingEngine() noexcept;

    void start() noexcept;
    void stop() noexcept;

private:
public: /// Debugging and monitoring fields. Will be removed after profiling and debugging.
    // =========================
    // CORE (LAYER 0)
    // =========================
    HashTables hashtables_;
    MarketBook marketbook_;
    Limits limits_;
    SessionManager session_manager_;
    TxPacketPoolManager txPkt_pool_;
    Parser_FIX parser_fix_;
    Builder_FIX builder_fix_;
    SoupBinTcp sbt_;
    LoginController login_;

    // =========================
    // QUEUES (LAYER 1)
    // =========================                         // Type Definition Location
    spscRxPacketQueue_t receiver_to_parser_;             // NetworkPackets.h
    spscMessageQueue_t parser_to_store_;                 // MessageWithVenue.h
    spscOrderQueue_t store_to_risk_;                     // Order.h
    spscOrderQueue_t store_to_strategy_;                 // Order.h
    spscOrderQueue_t store_to_strategy_free_slot_;       // Order.h
    spscOrderQueue_t strategy_to_risk_;                  // Order.h
    spscOrderQueue_t risk_to_builder_;                   // Order.h
    spscRejectOrderQueue_t risk_to_strategy_;            // RiskEngine.h
    spscTxPacketQueue_t builder_to_sender_;              // NetworkPackets.h
    spscDbQueue_t store_to_db_;                          // DbTypes.h
    spscDbQueue_t db_to_parser_;                         // DbTypes.h
    spscFIXTxSessionQueue_t parser_to_fixbuilder_tx_;    // FIXMessage.h
    spscFIXRxSessionQueue_t parser_to_fixbuilder_rx_;    // FIXMessage.h

    // =========================
    // COMPONENTS (LAYER 2)
    // =========================
    NetworkIO network_io_;
    Parser_Dispatch parser_dispatch_;
    OrderManager ord_mngr_;
    ClickhouseWriter ch_writer_;
//  StrategyEngine strategy_; Not Implemented Yet
    RiskEngine risk_;
    Builder_Dispatch builder_dispatch_;
    
    // =========================
    // FLOWS (LAYER 3)
    // =========================
    NetworkFlow network_flow_;
    ParserFlow parser_flow_;
    OrderFlow order_flow_;
    RiskFlow risk_flow_;
    BuilderFlow builder_flow_;

    std::atomic<bool> running_{false};
};

