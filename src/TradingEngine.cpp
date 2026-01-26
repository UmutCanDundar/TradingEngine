#include "TradingEngine.h"

TradingEngine::TradingEngine() noexcept
    : hashtables_(VENUE_COUNT),
      marketbook_(hashtables_),
      limits_(hashtables_),
      session_manager_(),
      inPkt_pool_(),
      fixBuilder_(session_manager_),
      sbt_(session_manager_),
      login_(sbt_, fixBuilder_, session_manager_),

      // ----- COMPONENTS -----
      network_io_(
          receiver_to_parser_,
          builder_to_sender_,
          sbt_,
          session_manager_,
          login_,
          inPkt_pool_),
      parser_dispatch_(
          receiver_to_parser_,
          parser_to_store_,
          parser_to_fixbuilder_out_,
          parser_to_fixbuilder_in_,
          session_manager_,
          db_to_parser_,
          network_io_),
      store_ram_(
          parser_to_store_,
          store_to_risk_,
          store_to_strategy_,
          store_to_strategy_free_slot_,
          store_to_db_,
          marketbook_,
          hashtables_),
      store_db_(
          store_to_db_,
          db_to_parser_),
      risk_engine_(
          store_to_risk_,
          strategy_to_risk_,
          risk_to_strategy_,
          risk_to_builder_,
          hashtables_,
          marketbook_,
          limits_,
          store_ram_),
      builder_dispatch_(
          builder_to_sender_,
          risk_to_builder_,
          parser_to_fixbuilder_out_,
          parser_to_fixbuilder_in_,
          session_manager_,
          sbt_,
          login_,
          inPkt_pool_,
          fixBuilder_),

      // ----- FLOWS -----
      network_flow_(network_io_),
      parser_flow_(parser_dispatch_),
      order_flow_(store_ram_, store_db_),
      risk_flow_(risk_engine_),
      builder_flow_(builder_dispatch_)
{
}

TradingEngine::~TradingEngine() noexcept
{
    stop();
}

void TradingEngine::start() noexcept
{
    running_.store(true, std::memory_order_release);

    network_flow_.start();
    parser_flow_.start();
    order_flow_.start();
    risk_flow_.start();
    builder_flow_.start();
}

void TradingEngine::stop() noexcept
{
    running_.store(false, std::memory_order_release);

    network_flow_.stop();
    parser_flow_.stop();
    order_flow_.stop();
    risk_flow_.stop();
    builder_flow_.stop();
}
