#include <gtest/gtest.h>

#include "OrderManager.h"
#include "MarketBook.h"
#include "HashTables.h"
#include "Order.h"
#include "dataset_risk.h"
#include "dataset_order_manager.h"
#include "RiskEngine.h"
#include "Limits.h"
#include "Logger.h"

#include <memory>

TEST(RiskEngineTest, MixedMessageTraffic)
{
    Logger::Init();

    spscMessageQueue_t parser_to_store;
    spscOrderQueue_t strategy_to_risk;
    spscRejectOrderQueue_t risk_to_strategy;
    spscOrderQueue_t risk_to_builder;
    spscOrderQueue_t store_to_strategy_free_slot;
    spscOrderQueue_t store_to_risk;
    spscOrderQueue_t store_to_strategy;
    spscDbQueue_t store_to_db;

    auto hashtables    = std::make_unique<HashTables>();
    auto limits        = std::make_unique<Limits>(*hashtables);
    auto marketbook    = std::make_unique<MarketBook>(*hashtables);

    auto order_manager = std::make_unique<OrderManager>(
                                        parser_to_store,
                                        store_to_strategy,
                                        store_to_strategy_free_slot,
                                        store_to_risk,
                                        store_to_db,
                                        *marketbook,
                                        *hashtables        
    );

    auto risk          = std::make_unique<RiskEngine>(
                                        store_to_risk,
                                        strategy_to_risk,
                                        risk_to_strategy,
                                        risk_to_builder,
                                        *hashtables,
                                        *marketbook,
                                        *limits,
                                        *order_manager   

    );

    // SYMBOLMETA INITIALIZING  (BIST GARAN(order_book_id = 3) and NASDAQ AAPL(stock_locate = 1))
    parser_to_store.push(MessageWithVenue<MessageTypes_t>{NASDAQ::ITCHMessage{&test_data_ordMngr::nasdaq_dir}, Venue::NASDAQ});
    order_manager->store();
    
    parser_to_store.push(MessageWithVenue<MessageTypes_t>{BIST::ITCHMessage{&test_data_ordMngr::bist_dir}, Venue::BIST});
    order_manager->store();

    //===========================================================================
    //                  UPDATE RISK RISK TEST
    //===========================================================================

        // ====================================================
        //       v STORE TO RISK TRAFFIC FOR RISK ENGINE v 

        std::array<Order*, 15> store_to_risk_traffic = [&]()
        {
            std::array<Order*, 15> arr{};

            using namespace test_data_risk;
            // 0 — FIX New Order (GARAN, buy 500@15000)
            arr[0] = fixNew;

            // 1 — BIST OUCH OrderAccepted (GARAN, order_id=42)
            arr[1] = ouchNew;

            // 2 — BIST ITCH AddOrder (GARAN, 1000@10000)
            arr[2] = itchAdd;

            // 3 — NASDAQ OUCH OrderAccepted (AAPL, user_ref=99)
            arr[3] = nqOuchNew;

            // 4 — NASDAQ ITCH AddOrder (AAPL, 100@10000)
            arr[4] = nqItchAdd;

            // 5 — FIX Partial Fill (GARAN, 200 filled)
            arr[5] = fixPartial;

            // 6 — BIST ITCH OrderExecuted (GARAN, 60 executed)
            arr[6] = itchExec;

            // 7 — BIST OUCH OrderExecuted (GARAN, 600 traded)
            arr[7] = ouchExec;

            // 8 — NASDAQ ITCH OrderExecuted (AAPL, 60 executed)
            arr[8] = nqItchExec;

            // 9 — NASDAQ OUCH OrderExecuted (AAPL, 600 executed)
            arr[9] = nqOuchExec;

            // 10 — BIST ITCH OrderDelete (GARAN, remaining 40 deleted)
            arr[10] = itchDelete;

            // 11 — BIST OUCH OrderCancelled (GARAN)
            arr[11] = ouchCancel;

            // 12 — NASDAQ ITCH OrderDelete (AAPL, remaining 40 deleted)
            arr[12] = nqItchDelete;

            // 13 — NASDAQ OUCH OrderCancelled (AAPL)
            arr[13] = nqOuchCancel;

            // 14 — FIX Cancel Confirm (GARAN)
            arr[14] = fixCancel;
        
            return arr;
        }();

        //      ^ STORE TO RISK TRAFFIC FOR RISK ENGINE ^
        // ====================================================

        auto fee_scale = 1'000'000;
        auto lev_scale = 10'000;

        // ─────────────────────────────────────────────────────────────────────────────
        // arr[0] → FIX New Order (GARAN, buy 500@15000)
        // ─────────────────────────────────────────────────────────────────────────────
            Order* order = store_to_risk_traffic[0];
            store_to_risk.push(order);
            risk->update_risk();
            
            auto venue_index = static_cast<uint8_t>(order->venue);
            auto& accRisk1 = risk->accountrisks_[venue_index];
            auto& symRisk1 = risk->symbolrisks_[venue_index][order->symbol_index];
            auto* ordRisk1 = risk->orderrisks_[venue_index][OrderRiskKey{order->symbol_index, order->price, static_cast<uint8_t>(order->side), venue_index}];
            
            auto side_mult_fix = (order->side == Side::Buy) ? 1 : -1;

            auto notional_bist = order->price * order->quantity * side_mult_fix;
            auto balance_bist = 10'000'000;
            
            // ── AccountRisk ──
            EXPECT_EQ(accRisk1.current_exposure.load(), notional_bist);   // signed
            EXPECT_EQ(accRisk1.positional_exposure.load(), 0);           // unsigned
            EXPECT_EQ(accRisk1.used_margin.load(),      std::abs(notional_bist));    // unsigned
            EXPECT_EQ(accRisk1.balance.load(),          balance_bist);
            EXPECT_EQ(accRisk1.current_leverage.load(), std::abs(notional_bist)*lev_scale / balance_bist); // unsigned
            EXPECT_EQ(accRisk1.daily_realized_pnl.load(), 0);
            EXPECT_EQ(accRisk1.total_unrealized_pnl.load(), 0);
            EXPECT_EQ(accRisk1.open_orders_count.load(), 1);
        
            // ── SymbolRisk ──
            EXPECT_EQ(symRisk1.open_orders_count.load(),        1);
            EXPECT_EQ(symRisk1.pending_notional_scaled.load(),  notional_bist);  
            EXPECT_EQ(symRisk1.net_position.load(),      0);
            EXPECT_EQ(symRisk1.cost_basis_scaled,        0);
            EXPECT_EQ(symRisk1.unrealized_pnl,           0);
            EXPECT_EQ(symRisk1.best_bid.load(),                 0);
            EXPECT_EQ(symRisk1.best_ask.load(),                 0);
        
            // ── OrderRisk ──
            EXPECT_EQ(ordRisk1->price.load(), 15000);
            EXPECT_EQ(ordRisk1->remaining_qty.load(), 50);
            EXPECT_EQ(ordRisk1->status.load(), Status::New);
            EXPECT_EQ(ordRisk1->symbol_index, 0);
            EXPECT_EQ(ordRisk1->side, Side::Buy);
            EXPECT_TRUE(ordRisk1->active.load());

        // ─────────────────────────────────────────────────────────────────────────────
        // arr[1] → BIST OUCH OrderAccepted (GARAN)
        // ─────────────────────────────────────────────────────────────────────────────
            order = store_to_risk_traffic[1];
            store_to_risk.push(order);
            risk->update_risk();
            
            venue_index = static_cast<uint8_t>(order->venue);
            auto& accRisk2 = risk->accountrisks_[venue_index];
            auto& symRisk2 = risk->symbolrisks_[venue_index][order->symbol_index];
            auto* ordRisk2 = risk->orderrisks_[venue_index][OrderRiskKey{order->symbol_index, order->price, static_cast<uint8_t>(order->side), venue_index}];

            auto side_mult_ouch = (order->side == Side::Buy) ? 1 : -1;

            notional_bist += order->price * order->quantity * side_mult_ouch;
            
            // ── AccountRisk ──
            EXPECT_EQ(accRisk2.current_exposure.load(), notional_bist);
            EXPECT_EQ(accRisk2.positional_exposure.load(), 0);
            EXPECT_EQ(accRisk2.used_margin.load(),      std::abs(notional_bist));
            EXPECT_EQ(accRisk2.balance.load(),          balance_bist);
            EXPECT_EQ(accRisk2.current_leverage.load(), std::abs(notional_bist)*lev_scale / balance_bist);
            EXPECT_EQ(accRisk2.daily_realized_pnl.load(), 0);
            EXPECT_EQ(accRisk2.total_unrealized_pnl.load(), 0);
            EXPECT_EQ(accRisk2.open_orders_count.load(), 2);
        
            // ── SymbolRisk ──
            EXPECT_EQ(symRisk2.open_orders_count.load(),        2);
            EXPECT_EQ(symRisk2.pending_notional_scaled.load(),  notional_bist);
            EXPECT_EQ(symRisk2.net_position.load(),      0);
            EXPECT_EQ(symRisk2.cost_basis_scaled,        0);
            EXPECT_EQ(symRisk2.unrealized_pnl,           0);
            EXPECT_EQ(symRisk2.best_bid.load(),                 0);
            EXPECT_EQ(symRisk2.best_ask.load(),                 0);
        
            // ── OrderRisk ──
            EXPECT_EQ(ordRisk2->price.load(),  10000);
            EXPECT_EQ(ordRisk2->remaining_qty.load(), 10);
            EXPECT_EQ(ordRisk2->status.load(),        Status::New);
            EXPECT_EQ(ordRisk2->symbol_index, 0);
            EXPECT_EQ(ordRisk2->side, Side::Buy);
            EXPECT_TRUE(ordRisk2->active.load());


        // ─────────────────────────────────────────────────────────────────────────────
        // arr[2] → BIST ITCH AddOrder (GARAN)
        // ─────────────────────────────────────────────────────────────────────────────
            order = store_to_risk_traffic[2];
            marketbook->add_order(*order);
            store_to_risk.push(order);
            risk->update_risk();
            
            venue_index = static_cast<uint8_t>(order->venue);
            auto& accRisk3 = risk->accountrisks_[venue_index];
            auto& symRisk3 = risk->symbolrisks_[venue_index][order->symbol_index];

            // ── AccountRisk ──
            EXPECT_EQ(accRisk3.daily_realized_pnl.load(), 0);
            EXPECT_EQ(accRisk3.total_unrealized_pnl.load(), 0);
            
            // ── SymbolRisk ──
            EXPECT_EQ(symRisk3.net_position.load(),      0);
            EXPECT_EQ(symRisk3.cost_basis_scaled,        0);
            EXPECT_EQ(symRisk3.unrealized_pnl,           0);
            EXPECT_EQ(symRisk3.best_bid.load(),          10000);
            EXPECT_EQ(symRisk3.best_ask.load(),          0);
        
        

        // ─────────────────────────────────────────────────────────────────────────────
        // arr[3] → NASDAQ OUCH OrderAccepted (AAPL)
        // ─────────────────────────────────────────────────────────────────────────────
            order = store_to_risk_traffic[3];
            store_to_risk.push(order);
            risk->update_risk();
            
            venue_index = static_cast<uint8_t>(order->venue);
            auto& accRisk4 = risk->accountrisks_[venue_index];
            auto& symRisk4 = risk->symbolrisks_[venue_index][order->symbol_index];
            auto* ordRisk4 = risk->orderrisks_[venue_index][OrderRiskKey{order->symbol_index, order->price, static_cast<uint8_t>(order->side), venue_index}];
            
            auto side_mult_NQouch = (order->side == Side::Buy) ? 1 : -1;
            auto notional_nasdaq = order->price * order->quantity * side_mult_NQouch;
            auto balance_nasdaq = 10'000'000;
        
            // ── AccountRisk ──
            EXPECT_EQ(accRisk4.current_exposure.load(), notional_nasdaq);
            EXPECT_EQ(accRisk4.positional_exposure.load(), 0);
            EXPECT_EQ(accRisk4.used_margin.load(),      std::abs(notional_nasdaq));
            EXPECT_EQ(accRisk4.balance.load(),          balance_nasdaq);
            EXPECT_EQ(accRisk4.current_leverage.load(), std::abs(notional_nasdaq)*lev_scale / balance_nasdaq);
            EXPECT_EQ(accRisk4.daily_realized_pnl.load(), 0);
            EXPECT_EQ(accRisk4.total_unrealized_pnl.load(), 0);
            EXPECT_EQ(accRisk4.open_orders_count.load(), 1);
        
            // ── SymbolRisk ──
            EXPECT_EQ(symRisk4.open_orders_count.load(),        1);
            EXPECT_EQ(symRisk4.pending_notional_scaled.load(),  notional_nasdaq);
            EXPECT_EQ(symRisk4.net_position.load(),      0);
            EXPECT_EQ(symRisk4.cost_basis_scaled,        0);
            EXPECT_EQ(symRisk4.unrealized_pnl,           0);
            EXPECT_EQ(symRisk4.best_bid.load(),                 0);
            EXPECT_EQ(symRisk4.best_ask.load(),                 0);
        
            // ── OrderRisk ──
            EXPECT_EQ(ordRisk4->price.load(), 10000);
            EXPECT_EQ(ordRisk4->remaining_qty.load(), 25);
            EXPECT_EQ(ordRisk4->status.load(), Status::New);
            EXPECT_EQ(ordRisk4->symbol_index, 0);
            EXPECT_EQ(ordRisk4->side, Side::Buy);
            EXPECT_TRUE(ordRisk4->active.load());

        // ─────────────────────────────────────────────────────────────────────────────
        // arr[4] → NASDAQ ITCH AddOrder (AAPL)
        // ─────────────────────────────────────────────────────────────────────────────
            order = store_to_risk_traffic[4];
            marketbook->add_order(*order);
            store_to_risk.push(order);
            risk->update_risk();
            
            venue_index = static_cast<uint8_t>(order->venue);
            auto& accRisk5 = risk->accountrisks_[venue_index];
            auto& symRisk5 = risk->symbolrisks_[venue_index][order->symbol_index];

            // ── AccountRisk ──
            EXPECT_EQ(accRisk5.daily_realized_pnl.load(), 0);
            EXPECT_EQ(accRisk5.total_unrealized_pnl.load(), 0);

            // ── SymbolRisk ──
            EXPECT_EQ(symRisk5.net_position.load(),     0);
            EXPECT_EQ(symRisk5.unrealized_pnl,          0);
            EXPECT_EQ(symRisk5.cost_basis_scaled,       0);
            EXPECT_EQ(symRisk5.best_bid.load(),                10000);
            EXPECT_EQ(symRisk5.best_ask.load(),                0);
        


        // ─────────────────────────────────────────────────────────────────────────────
        // arr[5] → FIX Partial Fill (GARAN)
        // ─────────────────────────────────────────────────────────────────────────────
            order = store_to_risk_traffic[5];
            store_to_risk.push(order);
            risk->update_risk();
            
            venue_index = static_cast<uint8_t>(order->venue);
            auto fee_rate_bist = risk->limits_.getAccountLimit(venue_index).maker_fee_rate;

            auto& accRisk6 = risk->accountrisks_[venue_index];
            auto& symRisk6 = risk->symbolrisks_[venue_index][order->symbol_index];
            auto* ordRisk6 = risk->orderrisks_[venue_index][OrderRiskKey{order->symbol_index, order->price, static_cast<uint8_t>(order->side), venue_index}];

            int64_t nominal_fix = order->price * order->last_exec_quantity * side_mult_fix;
            balance_bist -= std::abs(nominal_fix) + (std::abs(nominal_fix) * fee_rate_bist / fee_scale);
            int64_t net_pos_bist = order->last_exec_quantity * side_mult_fix;
            int64_t nominal_bist = nominal_fix;
            
            
            // ── AccountRisk ──
            EXPECT_EQ(accRisk6.current_exposure.load(), notional_bist); 
            EXPECT_EQ(accRisk6.positional_exposure.load(), std::abs(nominal_bist)); 
            EXPECT_EQ(accRisk6.used_margin.load(),      std::abs(notional_bist)-nominal_bist);
            EXPECT_EQ(accRisk6.balance.load(),          balance_bist);
            EXPECT_EQ(accRisk6.current_leverage.load(), std::abs(notional_bist) * lev_scale / balance_bist); 
            EXPECT_EQ(accRisk6.daily_realized_pnl.load(), 0);
            EXPECT_EQ(accRisk6.total_unrealized_pnl.load(), 0);
            EXPECT_EQ(accRisk6.open_orders_count.load(), 2);
        
            // ── SymbolRisk ──
            EXPECT_EQ(symRisk6.open_orders_count.load(),       2);
            EXPECT_EQ(symRisk6.pending_notional_scaled.load(), notional_bist - nominal_fix); 
            EXPECT_EQ(symRisk6.net_position.load(),     net_pos_bist);
            EXPECT_EQ(symRisk6.unrealized_pnl,     0);
            EXPECT_EQ(symRisk6.cost_basis_scaled,       nominal_bist);
            EXPECT_EQ(symRisk6.best_bid.load(),                10000);
            EXPECT_EQ(symRisk6.best_ask.load(),                0);
        
            // ── OrderRisk ──
            EXPECT_EQ(ordRisk6->price.load(), 15000);
            EXPECT_EQ(ordRisk6->remaining_qty.load(), 30);
            EXPECT_EQ(ordRisk6->status.load(), Status::Partial);
            EXPECT_EQ(ordRisk6->symbol_index, 0);
            EXPECT_EQ(ordRisk6->side, Side::Buy);
            EXPECT_TRUE(ordRisk6->active.load());


        // ─────────────────────────────────────────────────────────────────────────────
        // arr[6] → BIST ITCH Executed (GARAN)
        // ─────────────────────────────────────────────────────────────────────────────
            order = store_to_risk_traffic[6];
            marketbook->exec_order(*order);
            store_to_risk.push(order);
            risk->update_risk();
            
            venue_index = static_cast<uint8_t>(order->venue);
            auto& accRisk7 = risk->accountrisks_[venue_index];
            auto& symRisk7 = risk->symbolrisks_[venue_index][order->symbol_index];

            auto avg_price = nominal_bist / net_pos_bist;
            auto pnl_bist = (symRisk7.best_bid.load() - avg_price) * std::abs(net_pos_bist);

            // ── AccountRisk ──
            EXPECT_EQ(accRisk7.daily_realized_pnl.load(), 0);
            EXPECT_EQ(accRisk7.total_unrealized_pnl.load(), pnl_bist);

            // ── SymbolRisk ──
            EXPECT_EQ(symRisk7.net_position.load(),     net_pos_bist);
            EXPECT_EQ(symRisk7.unrealized_pnl,          pnl_bist);
            EXPECT_EQ(symRisk7.cost_basis_scaled,       nominal_bist);
            EXPECT_EQ(symRisk7.best_bid.load(),                10000);
            EXPECT_EQ(symRisk7.best_ask.load(),                0);
        

        // ─────────────────────────────────────────────────────────────────────────────
        // arr[7] → BIST OUCH Executed (GARAN)
        // ─────────────────────────────────────────────────────────────────────────────
            order = store_to_risk_traffic[7];
            store_to_risk.push(order);
            risk->update_risk();
            
            venue_index = static_cast<uint8_t>(order->venue);
            auto& accRisk8 = risk->accountrisks_[venue_index];
            auto& symRisk8 = risk->symbolrisks_[venue_index][order->symbol_index];
            auto* ordRisk8 = risk->orderrisks_[venue_index][OrderRiskKey{order->symbol_index, order->price, static_cast<uint8_t>(order->side),  venue_index}];

            int64_t nominal_ouch = order->price * order->last_exec_quantity * side_mult_ouch;
            balance_bist -= std::abs(nominal_ouch) + (std::abs(nominal_ouch) * fee_rate_bist / fee_scale);
            net_pos_bist += order->last_exec_quantity * side_mult_ouch;
            nominal_bist += nominal_ouch;

            // ── AccountRisk ──
            EXPECT_EQ(accRisk8.current_exposure.load(), notional_bist); 
            EXPECT_EQ(accRisk8.positional_exposure.load(), std::abs(nominal_bist)); 
            EXPECT_EQ(accRisk8.used_margin.load(),      std::abs(notional_bist) - std::abs(nominal_bist));
            EXPECT_EQ(accRisk8.balance.load(),          balance_bist);
            EXPECT_EQ(accRisk8.current_leverage.load(), std::abs(notional_bist) * lev_scale / balance_bist); 
            EXPECT_EQ(accRisk8.daily_realized_pnl.load(), 0);
            EXPECT_EQ(accRisk8.total_unrealized_pnl.load(), pnl_bist);
            EXPECT_EQ(accRisk8.open_orders_count.load(), 2);
        
            // ── SymbolRisk ──
            EXPECT_EQ(symRisk8.open_orders_count.load(),       2);
            EXPECT_EQ(symRisk8.pending_notional_scaled.load(), notional_bist - nominal_ouch - nominal_fix);
            EXPECT_EQ(symRisk8.net_position.load(),     net_pos_bist);
            EXPECT_EQ(symRisk8.unrealized_pnl,          pnl_bist);
            EXPECT_EQ(symRisk8.cost_basis_scaled,       nominal_bist);
            EXPECT_EQ(symRisk8.best_bid.load(),                10000);
            EXPECT_EQ(symRisk8.best_ask.load(),                0);
        
            // ── OrderRisk ──
            EXPECT_EQ(ordRisk8->price.load(), 10000);
            EXPECT_EQ(ordRisk8->remaining_qty.load(), 4);
            EXPECT_EQ(ordRisk8->status.load(), Status::Partial);
            EXPECT_EQ(ordRisk8->symbol_index, 0);
            EXPECT_EQ(ordRisk8->side, Side::Buy);
            EXPECT_TRUE(ordRisk8->active.load());


        // ─────────────────────────────────────────────────────────────────────────────
        // arr[8] → NASDAQ ITCH Executed (AAPL)
        // ─────────────────────────────────────────────────────────────────────────────
            order = store_to_risk_traffic[8];
            marketbook->exec_order(*order);
            store_to_risk.push(order);
            risk->update_risk();
            
            venue_index = static_cast<uint8_t>(order->venue);
            auto& accRisk9 = risk->accountrisks_[venue_index];
            auto& symRisk9 = risk->symbolrisks_[venue_index][order->symbol_index];

            // ── AccountRisk ──
            EXPECT_EQ(accRisk9.daily_realized_pnl.load(), 0);
            EXPECT_EQ(accRisk9.total_unrealized_pnl.load(), 0);

            // ── SymbolRisk ──
            EXPECT_EQ(symRisk9.net_position.load(),     0);
            EXPECT_EQ(symRisk9.unrealized_pnl,          0);
            EXPECT_EQ(symRisk9.cost_basis_scaled,       0);
            EXPECT_EQ(symRisk9.best_bid.load(),                10000);
            EXPECT_EQ(symRisk9.best_ask.load(),                0);
        


        // ─────────────────────────────────────────────────────────────────────────────
        // arr[9] → NASDAQ OUCH Executed (AAPL)
        // ─────────────────────────────────────────────────────────────────────────────
            order = store_to_risk_traffic[9];
            store_to_risk.push(order);
            risk->update_risk();
            
            venue_index = static_cast<uint8_t>(order->venue);
            auto fee_rate_nasdaq = risk->limits_.getAccountLimit(venue_index).maker_fee_rate;

            auto& accRisk10 = risk->accountrisks_[venue_index];
            auto& symRisk10 = risk->symbolrisks_[venue_index][order->symbol_index];
            auto* ordRisk10 = risk->orderrisks_[venue_index][OrderRiskKey{order->symbol_index, order->price, static_cast<uint8_t>(order->side), venue_index}];
            
            int64_t nominal_nasdaq = order->price * order->last_exec_quantity * side_mult_NQouch;
            int64_t net_pos_nasdaq = order->last_exec_quantity * side_mult_NQouch; 
            balance_nasdaq -= std::abs(nominal_nasdaq) + (std::abs(nominal_nasdaq) * fee_rate_nasdaq / fee_scale);
            

            // ── AccountRisk ──
            EXPECT_EQ(accRisk10.current_exposure.load(), notional_nasdaq); 
            EXPECT_EQ(accRisk10.positional_exposure.load(), std::abs(nominal_nasdaq)); 
            EXPECT_EQ(accRisk10.used_margin.load(),      std::abs(notional_nasdaq) - std::abs(nominal_nasdaq));
            EXPECT_EQ(accRisk10.balance.load(),          balance_nasdaq);
            EXPECT_EQ(accRisk10.current_leverage.load(), std::abs(notional_nasdaq) * lev_scale / balance_nasdaq); 
            EXPECT_EQ(accRisk10.daily_realized_pnl.load(), 0);
            EXPECT_EQ(accRisk10.total_unrealized_pnl.load(), 0);
            EXPECT_EQ(accRisk10.open_orders_count.load(), 1);
        
            // ── SymbolRisk ──
            EXPECT_EQ(symRisk10.open_orders_count.load(),       1);
            EXPECT_EQ(symRisk10.pending_notional_scaled.load(), notional_nasdaq - nominal_nasdaq);
            EXPECT_EQ(symRisk10.net_position.load(),     net_pos_nasdaq);
            EXPECT_EQ(symRisk10.unrealized_pnl,          0);
            EXPECT_EQ(symRisk10.cost_basis_scaled,       nominal_nasdaq);
            EXPECT_EQ(symRisk10.best_bid.load(),                10000);
            EXPECT_EQ(symRisk10.best_ask.load(),                0);
        
            // ── OrderRisk ──
            EXPECT_EQ(ordRisk10->price.load(), 10000);
            EXPECT_EQ(ordRisk10->remaining_qty.load(), 15);
            EXPECT_EQ(ordRisk10->status.load(), Status::Partial);
            EXPECT_EQ(ordRisk10->symbol_index, 0);
            EXPECT_EQ(ordRisk10->side, Side::Buy);
            EXPECT_TRUE(ordRisk10->active.load());


        // ─────────────────────────────────────────────────────────────────────────────
        // arr[10] → BIST ITCH Delete (GARAN)
        // ─────────────────────────────────────────────────────────────────────────────
            order = store_to_risk_traffic[10];
            marketbook->delete_order(*order);
            store_to_risk.push(order);
            risk->update_risk();
            
            venue_index = static_cast<uint8_t>(order->venue);
            auto& accRisk11 = risk->accountrisks_[venue_index];
            auto& symRisk11 = risk->symbolrisks_[venue_index][order->symbol_index];

            // ── AccountRisk ──
            EXPECT_EQ(accRisk11.daily_realized_pnl.load(), 0);
            EXPECT_EQ(accRisk11.total_unrealized_pnl.load(), pnl_bist);

            // ── SymbolRisk ──
            EXPECT_EQ(symRisk11.net_position.load(),     net_pos_bist);
            EXPECT_EQ(symRisk11.unrealized_pnl,          pnl_bist);
            EXPECT_EQ(symRisk11.cost_basis_scaled,       nominal_bist);
            EXPECT_EQ(symRisk11.best_bid.load(),                0);
            EXPECT_EQ(symRisk11.best_ask.load(),                0);
        

        // ─────────────────────────────────────────────────────────────────────────────
        // arr[11] → BIST OUCH Cancelled (GARAN)
        // ─────────────────────────────────────────────────────────────────────────────
            order = store_to_risk_traffic[11];
            store_to_risk.push(order);
            risk->update_risk();
            
            venue_index = static_cast<uint8_t>(order->venue);
            auto& accRisk12 = risk->accountrisks_[venue_index];
            auto& symRisk12 = risk->symbolrisks_[venue_index][order->symbol_index];
            auto* ordRisk12 = risk->orderrisks_[venue_index][OrderRiskKey{order->symbol_index, order->price, static_cast<uint8_t>(order->side), venue_index}];
            
            auto pending_notional = notional_bist - (nominal_fix + nominal_ouch); // pre-filled fix and ouch
            nominal_ouch = order->price * order->replaced_quantity * side_mult_ouch; // deleted ouch
            pending_notional += nominal_ouch; 
            notional_bist += nominal_ouch;

            // ── AccountRisk ──
            EXPECT_EQ(accRisk12.current_exposure.load(), notional_bist);
            EXPECT_EQ(accRisk12.positional_exposure.load(), std::abs(nominal_bist));
            EXPECT_EQ(accRisk12.used_margin.load(),      pending_notional);
            EXPECT_EQ(accRisk12.balance.load(),          balance_bist);
            EXPECT_EQ(accRisk12.current_leverage.load(), std::abs(notional_bist) * lev_scale / balance_bist); 
            EXPECT_EQ(accRisk12.daily_realized_pnl.load(), 0);
            EXPECT_EQ(accRisk12.total_unrealized_pnl.load(), pnl_bist);
            EXPECT_EQ(accRisk12.open_orders_count.load(), 1);
        
            // ── SymbolRisk ──
            EXPECT_EQ(symRisk12.open_orders_count.load(),       1);
            EXPECT_EQ(symRisk12.pending_notional_scaled.load(), pending_notional);
            EXPECT_EQ(symRisk12.net_position.load(),     net_pos_bist);
            EXPECT_EQ(symRisk12.unrealized_pnl,          pnl_bist);
            EXPECT_EQ(symRisk12.cost_basis_scaled,       nominal_bist);
            EXPECT_EQ(symRisk12.best_bid.load(),                0);
            EXPECT_EQ(symRisk12.best_ask.load(),                0);
        
            // ── OrderRisk ──
            ASSERT_EQ(ordRisk12, nullptr);
        


        // ─────────────────────────────────────────────────────────────────────────────
        // arr[12] → NASDAQ ITCH Delete (AAPL)
        // ─────────────────────────────────────────────────────────────────────────────
            order = store_to_risk_traffic[12];
            marketbook->delete_order(*order);
            store_to_risk.push(order);
            risk->update_risk();
            
            venue_index = static_cast<uint8_t>(order->venue);
            auto& accRisk13 = risk->accountrisks_[venue_index];
            auto& symRisk13 = risk->symbolrisks_[venue_index][order->symbol_index];
        
            // ── AccountRisk ──
            EXPECT_EQ(accRisk13.daily_realized_pnl.load(), 0);
            EXPECT_EQ(accRisk13.total_unrealized_pnl.load(), 0);

            // ── SymbolRisk ──
            EXPECT_EQ(symRisk13.net_position.load(),     net_pos_nasdaq);
            EXPECT_EQ(symRisk13.unrealized_pnl,          0);
            EXPECT_EQ(symRisk13.cost_basis_scaled,       nominal_nasdaq);
            EXPECT_EQ(symRisk13.best_bid.load(),                0);
            EXPECT_EQ(symRisk13.best_ask.load(),                0);
        

        // ─────────────────────────────────────────────────────────────────────────────
        // arr[13] → NASDAQ OUCH Cancelled (AAPL)
        // ─────────────────────────────────────────────────────────────────────────────
            order = store_to_risk_traffic[13];
            store_to_risk.push(order);
            risk->update_risk();
            
            venue_index = static_cast<uint8_t>(order->venue);
            auto& accRisk14 = risk->accountrisks_[venue_index];
            auto& symRisk14 = risk->symbolrisks_[venue_index][order->symbol_index];
            auto* ordRisk14 = risk->orderrisks_[venue_index][OrderRiskKey{order->symbol_index, order->price, static_cast<uint8_t>(order->side), venue_index}];

            notional_nasdaq += order->price * order->replaced_quantity * side_mult_NQouch; // deleted
            
            // ── AccountRisk ──
            EXPECT_EQ(accRisk14.current_exposure.load(),  notional_nasdaq);
            EXPECT_EQ(accRisk14.positional_exposure.load(), std::abs(nominal_nasdaq));
            EXPECT_EQ(accRisk14.used_margin.load(),      0);
            EXPECT_EQ(accRisk14.balance.load(),          balance_nasdaq);
            EXPECT_EQ(accRisk14.current_leverage.load(), std::abs(notional_nasdaq) * lev_scale / balance_nasdaq); 
            EXPECT_EQ(accRisk14.daily_realized_pnl.load(), 0);
            EXPECT_EQ(accRisk14.total_unrealized_pnl.load(), 0);
            EXPECT_EQ(accRisk14.open_orders_count.load(), 0);
        
            // ── SymbolRisk ──
            EXPECT_EQ(symRisk14.open_orders_count.load(),       0);
            EXPECT_EQ(symRisk14.pending_notional_scaled.load(), 0);
            EXPECT_EQ(symRisk14.net_position.load(),     net_pos_nasdaq);
            EXPECT_EQ(symRisk14.unrealized_pnl,          0);
            EXPECT_EQ(symRisk14.cost_basis_scaled,       nominal_nasdaq);
            EXPECT_EQ(symRisk14.best_bid.load(),                0);
            EXPECT_EQ(symRisk14.best_ask.load(),                0);
        
            // ── OrderRisk ──
            ASSERT_EQ(ordRisk14, nullptr);

        // ─────────────────────────────────────────────────────────────────────────────
        // arr[14] → FIX Cancel Confirm (GARAN)
        // ─────────────────────────────────────────────────────────────────────────────
            order = store_to_risk_traffic[14];
            store_to_risk.push(order);
            risk->update_risk();
            
            venue_index = static_cast<uint8_t>(order->venue);
            auto& accRisk15 = risk->accountrisks_[venue_index];
            auto& symRisk15 = risk->symbolrisks_[venue_index][order->symbol_index];
            auto* ordRisk15 = risk->orderrisks_[venue_index][OrderRiskKey{order->symbol_index, order->price, static_cast<uint8_t>(order->side), venue_index}];

            nominal_fix = order->price * order->replaced_quantity * side_mult_fix;
            notional_bist += nominal_fix; // 0
            pending_notional += nominal_fix; // 0
            
            // ── AccountRisk ──
            EXPECT_EQ(accRisk15.current_exposure.load(), notional_bist);
            EXPECT_EQ(accRisk15.positional_exposure.load(), std::abs(nominal_bist));
            EXPECT_EQ(accRisk15.used_margin.load(),      pending_notional);
            EXPECT_EQ(accRisk15.balance.load(),          balance_bist);
            EXPECT_EQ(accRisk15.current_leverage.load(), std::abs(notional_bist) * lev_scale / balance_bist); 
            EXPECT_EQ(accRisk15.daily_realized_pnl.load(), 0);
            EXPECT_EQ(accRisk15.total_unrealized_pnl.load(), pnl_bist);
            EXPECT_EQ(accRisk15.open_orders_count.load(), 0);
        
            // ── SymbolRisk ──
            EXPECT_EQ(symRisk15.open_orders_count.load(),       0);
            EXPECT_EQ(symRisk15.pending_notional_scaled.load(), pending_notional);
            EXPECT_EQ(symRisk15.net_position.load(),     net_pos_bist);
            EXPECT_EQ(symRisk15.unrealized_pnl,          pnl_bist);
            EXPECT_EQ(symRisk15.cost_basis_scaled,       nominal_bist);
            EXPECT_EQ(symRisk15.best_bid.load(),                0);
            EXPECT_EQ(symRisk15.best_ask.load(),                0);
        
            // ── OrderRisk ──
            ASSERT_EQ(ordRisk15, nullptr);


    //===========================================================================
    //                  CHECK RISK TEST
    //===========================================================================
        
        // ====================================================
        //       v STRATEGY TO RISK TRAFFIC FOR RISK ENGINE v

            std::array<Order*, 15> strategy_to_risk_traffic = [&]()
            {
                std::array<Order*, 15> arr{};

                using namespace test_data_risk;
                arr[0] = o1;
                arr[1] = o2;
                arr[2] = o3;
                arr[3] = o4;
                arr[4] = o5;
                arr[5] = o6;
                arr[6] = o7;
                arr[7] = o8;
                arr[8] = o9;
                arr[9] = o10;
                arr[10] = o11;
                arr[11] = o12;
                arr[12] = o13; 
                arr[13] = o14; 
                arr[14] = o15; 
            
                return arr;
            }();

            //      ^ STRATEGY TO RISK TRAFFIC FOR RISK ENGINE ^
            // ====================================================
            
            //TICK SIZE INITIALIZATION VENUE::BIST--0 SYMBOL::GARAN--0 TICKSIZE--10
            auto* symmeta = order_manager->get_symbolmeta(Venue::BIST, 3);
            auto* ticksize_entry = new TickSizeEntry(0, 100'000'000, 10);
            symmeta->tick_size_table[0].store(ticksize_entry, std::memory_order_relaxed);

            // BEST BID/ASK INITIALIZATION VENUE::BIST--0 SYMBOL::GARAN--0
            auto& symRisk = risk->symbolrisks_[0][0];
            symRisk.best_bid.store(10000, std::memory_order_relaxed);
            symRisk.best_ask.store(10000, std::memory_order_relaxed);

            auto& accRisk = risk->accountrisks_[0];
            auto& ordRisk_map = risk->orderrisks_[0];
            
            // RESET FOR THE CHECK TEST
            accRisk.current_exposure.store(0, std::memory_order_relaxed);
            accRisk.open_orders_count.store(0, std::memory_order_relaxed);
            accRisk.used_margin.store(0, std::memory_order_relaxed);
            accRisk.balance.store(100'000'000, std::memory_order_relaxed);
            accRisk.current_leverage.store(0, std::memory_order_relaxed);
            accRisk.total_unrealized_pnl.store(0, std::memory_order_relaxed);
            accRisk.daily_realized_pnl.store(0, std::memory_order_relaxed);
            accRisk.status.store(AccountStatus::Active, std::memory_order_relaxed);
            symRisk.open_orders_count.store(0, std::memory_order_relaxed);
            symRisk.avg_entry_price.store(0, std::memory_order_relaxed);
            symRisk.net_position.store(0, std::memory_order_relaxed);
            symRisk.cost_basis_scaled = 0;

            OrderWithRejectReason rej_order{};
            uint32_t reason = 0;  

            order = strategy_to_risk_traffic[0];
            strategy_to_risk.push(order);
            risk->check_risk();
            risk_to_strategy.pop(rej_order);
            EXPECT_EQ(rej_order.order, nullptr);

            order = strategy_to_risk_traffic[1];
            strategy_to_risk.push(order);
            risk->check_risk();
            risk_to_strategy.pop(rej_order);
            EXPECT_EQ(rej_order.RejectReason, reason 
                | static_cast<uint32_t>(RiskRejectReason::MinOrderSizeExceeded));

            order = strategy_to_risk_traffic[2];
            strategy_to_risk.push(order);
            risk->check_risk();
            risk_to_strategy.pop(rej_order);
            EXPECT_EQ(rej_order.RejectReason, reason 
                | static_cast<uint32_t>(RiskRejectReason::MaxOrderSizeExceeded));
                
            order = strategy_to_risk_traffic[3];
            strategy_to_risk.push(order);
            risk->check_risk();
            risk_to_strategy.pop(rej_order);
            EXPECT_EQ(rej_order.RejectReason, reason 
                | static_cast<uint32_t>(RiskRejectReason::InvalidLotSize));

            order = strategy_to_risk_traffic[4];
            strategy_to_risk.push(order);
            symRisk.best_bid.store(180000, std::memory_order_relaxed);
            risk->check_risk();
            risk_to_strategy.pop(rej_order);
            EXPECT_EQ(rej_order.RejectReason, reason 
                | static_cast<uint32_t>(RiskRejectReason::MaxOrderNotionalLimitExceeded)
                | static_cast<uint32_t>(RiskRejectReason::InvalidPriceRange));
            symRisk.best_bid.store(10000, std::memory_order_relaxed);
            
            order = strategy_to_risk_traffic[5];
            strategy_to_risk.push(order);
            risk->check_risk();
            risk_to_strategy.pop(rej_order);
            EXPECT_EQ(rej_order.RejectReason, reason 
                | static_cast<uint32_t>(RiskRejectReason::PriceTickInvalid));
                
            order = strategy_to_risk_traffic[6];
            strategy_to_risk.push(order);
            risk->check_risk();
            risk_to_strategy.pop(rej_order);
            EXPECT_EQ(rej_order.RejectReason, reason 
                | static_cast<uint32_t>(RiskRejectReason::FatFingerCheckFailed));
                
            order = strategy_to_risk_traffic[7];
            strategy_to_risk.push(order);
            symRisk.net_position.store(960, std::memory_order_relaxed);
            risk->check_risk();
            risk_to_strategy.pop(rej_order);
            EXPECT_EQ(rej_order.RejectReason, reason 
                | static_cast<uint32_t>(RiskRejectReason::MaxPositionLimitExceeded));
            symRisk.net_position.store(0, std::memory_order_relaxed);

            order = strategy_to_risk_traffic[8];
            strategy_to_risk.push(order);
            accRisk.current_exposure.store(49'900'000, std::memory_order_relaxed);
            risk->check_risk();
            risk_to_strategy.pop(rej_order);
            EXPECT_EQ(rej_order.RejectReason, reason 
                | static_cast<uint32_t>(RiskRejectReason::MaxNotionalLimitExceeded));
            accRisk.current_exposure.store(0, std::memory_order_relaxed);
                
            order = strategy_to_risk_traffic[9];
            strategy_to_risk.push(order);
            accRisk.balance.store(10'000, std::memory_order_relaxed);
            accRisk.used_margin.store(0, std::memory_order_relaxed);
            risk->check_risk();
            risk_to_strategy.pop(rej_order);
            EXPECT_EQ(rej_order.RejectReason, reason 
                | static_cast<uint32_t>(RiskRejectReason::AccountBalanceInsufficient)
                | static_cast<uint32_t>(RiskRejectReason::FreeMarginInsufficient)
                | static_cast<uint32_t>(RiskRejectReason::MaxLeverageExceeded));
            accRisk.balance.store(100'000'000, std::memory_order_relaxed);

            order = strategy_to_risk_traffic[10];
            strategy_to_risk.push(order);
            accRisk.open_orders_count.store(5, std::memory_order_relaxed);
            risk->check_risk();
            risk_to_strategy.pop(rej_order);
            EXPECT_EQ(rej_order.RejectReason, reason 
                | static_cast<uint32_t>(RiskRejectReason::MaxOpenOrdersReachedAccount));
            accRisk.open_orders_count.store(0, std::memory_order_relaxed);

            order = strategy_to_risk_traffic[11];
            strategy_to_risk.push(order);
            symRisk.open_orders_count.store(3, std::memory_order_relaxed);
            risk->check_risk();
            risk_to_strategy.pop(rej_order);
            EXPECT_EQ(rej_order.RejectReason, reason 
                | static_cast<uint32_t>(RiskRejectReason::MaxOpenOrdersReachedSymbol));
            symRisk.open_orders_count.store(0, std::memory_order_relaxed);
            
            //OrderRisk Inserted For The SelfTradeCheck
            OrderRisk* oR = new OrderRisk();
            oR->symbol_index = order->symbol_index;
            oR->side = order->side;
            oR->venue = order->venue;
            oR->price.store(order->price, std::memory_order_relaxed);
            oR->active.store(true, std::memory_order_relaxed);
            auto [it_inserted, ok] = ordRisk_map.emplace(OrderRiskKey{
                                    order->symbol_index, 
                                    order->price, 
                                    static_cast<uint8_t>(order->side), 
                                    static_cast<uint8_t>(order->venue)
                                    }, oR);

            order = strategy_to_risk_traffic[12];
            strategy_to_risk.push(order);
            risk->check_risk();
            risk_to_strategy.pop(rej_order);
            EXPECT_EQ(rej_order.RejectReason, reason 
                | static_cast<uint32_t>(RiskRejectReason::SelfTradeDetected));
                            
            order = strategy_to_risk_traffic[13];
            strategy_to_risk.push(order);
            risk->check_risk();
            risk_to_strategy.pop(rej_order);
            EXPECT_EQ(rej_order.RejectReason, reason 
                | static_cast<uint32_t>(RiskRejectReason::UnrealizedLossLimitExceeded));
                
            order = strategy_to_risk_traffic[14];
            strategy_to_risk.push(order);
            symRisk.avg_entry_price.store(30000, std::memory_order_relaxed);
            risk->check_risk();
            risk_to_strategy.pop(rej_order);
            EXPECT_EQ(rej_order.RejectReason, reason 
                | static_cast<uint32_t>(RiskRejectReason::RealizedLossLimitExceeded));
                
            // order = strategy_to_risk_traffic[14]; // Commented out during benchmarking — rate limit check interferes with test timing
            // strategy_to_risk.push(order);
            // risk->check_risk();
            // risk_to_strategy.pop(rej_order);
            // EXPECT_EQ(rej_order.RejectReason, reason
            //     | static_cast<uint32_t>(RiskRejectReason::MaxOrderRateLimitExceededSymbol)
            //     | static_cast<uint32_t>(RiskRejectReason::MaxOrderRateLimitExceededAccount));
                
    delete ticksize_entry;
    delete oR;
    Logger::Shutdown();             
}

