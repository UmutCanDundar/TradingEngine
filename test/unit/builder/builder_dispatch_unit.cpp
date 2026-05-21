#include <gtest/gtest.h>

#include "OrderManager.h"
#include "MarketBook.h"
#include "HashTables.h"
#include "Order.h"
#include "dataset_builder.h"
#include "SessionManager.h"
#include "LoginController.h"
#include "SoupBinTcp.h"
#include "Builder_FIX.h"
#include "Parser_FIX.h"
#include "NetworkPackets.h"
#include "FIXMessage.h"
#include "Logger.h"
#include "Builder_Dispatch.h"

#include <memory>

TEST(BuilderDispatchTest, MixedMessageTraffic)
{
    Logger::Init();

    auto txPkt_pool = std::make_unique<TxPacketPoolManager>();
    spscFIXTxSessionQueue_t parser_to_fixbuilder_tx;
    spscFIXRxSessionQueue_t parser_to_fixbuilder_rx;
    spscTxPacketQueue_t builder_to_sender;
    spscOrderQueue_t risk_to_builder;
    
    spscOrderQueue_t store_to_strategy_free_slot;
    spscOrderQueue_t store_to_risk;
    spscOrderQueue_t store_to_strategy;
    spscMessageQueue_t parser_to_store;
    spscDbQueue_t store_to_db;

    auto hashtables    = std::make_unique<HashTables>();
    auto marketbook    = std::make_unique<MarketBook>(*hashtables);
    auto sess_mngr     = std::make_unique<SessionManager>();
    auto sbt           = std::make_unique<SoupBinTcp>(*sess_mngr);
    auto builder_fix   = std::make_unique<Builder_FIX>(*sess_mngr);
    auto login         = std::make_unique<LoginController>(*sbt, *builder_fix, *sess_mngr);
    auto parser_fix    = std::make_unique<Parser_FIX>(parser_to_fixbuilder_tx);
    auto order_manager = std::make_unique<OrderManager>(
                                        parser_to_store,
                                        store_to_strategy,
                                        store_to_strategy_free_slot,
                                        store_to_risk,
                                        store_to_db,
                                        *marketbook,
                                        *hashtables        
    );

    auto builder_dispatch = std::make_unique<Builder_Dispatch>(
                                        builder_to_sender,
                                        risk_to_builder,
                                        parser_to_fixbuilder_rx,
                                        parser_to_fixbuilder_tx,
                                        *sess_mngr,
                                        *sbt,
                                        *login,
                                        *txPkt_pool,
                                        *builder_fix,
                                        *order_manager,
                                        *parser_fix        
    );

   
// ====================================================
//       v TRAFFIC FOR BUILDING DISPATCH v 
    
    std::array<Order*, 3> orders = {
        test_data_builder::fix_new_order, 
        test_data_builder::ouch_new_order,
        test_data_builder::NQouch_new_order
    };
    
    for (auto ord : orders) 
    {  
        risk_to_builder.push(ord);
        builder_dispatch->dispatch();
    }
//      ^ TRAFFIC FOR BUILDING DISPATCH ^
// ====================================================

    TxPacket* txPkt = nullptr;

    builder_to_sender.pop(txPkt);
    EXPECT_EQ(txPkt->data[0], '8'); 
    EXPECT_EQ(txPkt->len, 177);
    EXPECT_EQ(txPkt->sock_index, 0);  
    EXPECT_FALSE(txPkt->is_login_msg); 

    builder_to_sender.pop(txPkt);
    EXPECT_EQ(txPkt->data[33], 0x27); 
    EXPECT_EQ(txPkt->len, 117);
    EXPECT_EQ(txPkt->sock_index, 1);  
    EXPECT_FALSE(txPkt->is_login_msg); 
    
    builder_to_sender.pop(txPkt);
    EXPECT_EQ(txPkt->data[12], 0x64); 
    EXPECT_EQ(txPkt->len, 50);
    EXPECT_EQ(txPkt->sock_index, 2);  
    EXPECT_FALSE(txPkt->is_login_msg); 

    Logger::Shutdown();
}

