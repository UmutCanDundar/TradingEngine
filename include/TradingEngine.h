#pragma once

#include "Protocol-Venue.h"

#include <atomic>

#include "HashTables.h"
#include "MarketBook.h"
#include "Limits.h"
#include "SessionManager.h"
#include "LoginController.h"

// ===== COMPONENTS =====
#include "SoupBinTcp.h"
#include "NetworkIO.h"
#include "Parser_Dispatch.h"
#include "Store_RAM.h"
#include "Store_DB.h"
#include "RiskEngine.h"
#include "Builder_Dispatch.h"

// ===== FLOWS =====
#include "flows/NetworkFlow.h"
#include "flows/ParserFlow.h"
#include "flows/OrderFlow.h"
#include "flows/RiskFlow.h"
#include "flows/BuilderFlow.h"

class TradingEngine
{
public:
    TradingEngine() noexcept;
    ~TradingEngine() noexcept;

    void start() noexcept;
    void stop() noexcept;

private:
    // =========================
    // (KATMAN 0)
    // =========================
    HashTables hashtables_;
    MarketBook marketbook_;
    Limits limits_;
    SessionManager session_manager_;
    LoginController login_;
    InPacketPoolManager inPkt_pool_;
    Builder_FIX fixBuilder_;
    SoupBinTcp sbt_;
   
    // =========================
    // QUEUES (KATMAN 1)
    // =========================
    spscPacketQueue_t receiver_to_parser_;
    spscMessageQueue_t parser_to_store_;
    spscOrderQueue_t store_to_risk_;
    spscOrderQueue_t store_to_strategy_;
    spscOrderQueue_t store_to_strategy_free_slot_;
    spscOrderQueue_t strategy_to_risk_;
    spscRejectOrderQueue_t risk_to_strategy_;
    spscOrderQueue_t risk_to_builder_;
    spscInPacketQueue_t builder_to_sender_;
    spscDbQueue_t store_to_db_;
    spscDbQueue_t db_to_parser_;
    spscFIXInSessionQueue_t parser_to_fixbuilder_in_;
    spscFIXOutSessionQueue_t parser_to_fixbuilder_out_;

    // =========================
    // COMPONENTS (KATMAN 2)
    // =========================
    NetworkIO network_io_;
    Parser_Dispatch parser_dispatch_;
    Store_RAM store_ram_;
    Store_DB store_db_;
    RiskEngine risk_engine_;
    Builder_Dispatch builder_dispatch_;

    // =========================
    // FLOWS (KATMAN 3)
    // =========================
    NetworkFlow network_flow_;
    ParserFlow parser_flow_;
    OrderFlow order_flow_;
    RiskFlow risk_flow_;
    BuilderFlow builder_flow_;

    std::atomic<bool> running_{false};
};

