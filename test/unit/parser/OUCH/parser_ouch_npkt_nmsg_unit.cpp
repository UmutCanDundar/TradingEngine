#include <gtest/gtest.h>

#include "Parser_Dispatch.h"
#include "SessionManager.h"
#include "LoginController.h"
#include "SoupBinTcp.h"
#include "Builder_FIX.h"
#include "Parser_FIX.h"
#include "NetworkIO.h"
#include "dataset_parser.h"

#include <memory>

TEST(OuchParserDispatchTest, MultOuchPkt)
{
    std::atomic<bool> running{true};

    auto txPkt_pool = std::make_unique<TxPacketPoolManager>();
    spscFIXTxSessionQueue_t parser_to_fixbuilder_tx;
    spscRxPacketQueue_t receiver_to_parser;
    spscTxPacketQueue_t builder_to_sender;
    spscMessageQueue_t parser_to_store; 
    spscFIXRxSessionQueue_t parser_to_fixbuilder_rx;
    spscDbQueue_t db_to_parser; 
    auto sess_mngr     = std::make_unique<SessionManager>();
    auto sbt           = std::make_unique<SoupBinTcp>(*sess_mngr);
    auto builder_fix   = std::make_unique<Builder_FIX>(*sess_mngr);
    auto login         = std::make_unique<LoginController>(*sbt, *builder_fix, *sess_mngr);
    auto network_io    = std::make_unique<NetworkIO>(receiver_to_parser, builder_to_sender, *sess_mngr, *sbt, *login, *txPkt_pool, running);
    auto parser_fix    = std::make_unique<Parser_FIX>(parser_to_fixbuilder_tx);

    auto parser_dispatch = std::make_unique<Parser_Dispatch>(
                                        receiver_to_parser,
                                        parser_to_store,
                                        parser_to_fixbuilder_rx,
                                        parser_to_fixbuilder_tx,
                                        *sess_mngr,
                                        db_to_parser,
                                        *network_io,
                                        *parser_fix
    );
    uint8_t sess_index = sess_mngr->getSessionIndex(Venue::BIST, Protocol::OUCH);
    
    RxPacket* rxPkt;
    PendingQueue<RxPacket *, 256> pend_read;

// ====================================================
//       v DIFFERENT SCENARIOS FOR OUCH PARSING v 
    
// SCENARIO 1: THREE PACKETS CONTAINING PARTIAL OUCH MESSAGES 
    std::array<RxPacket*, 5> pkts = {
        &test_data_parser::ouch_bist_RxPacket_partial_1, 
        &test_data_parser::ouch_bist_RxPacket_partial_2, 
        &test_data_parser::ouch_bist_RxPacket_partial_3,
        &test_data_parser::ouch_bist_RxPacket_partial_4, 
        &test_data_parser::ouch_bist_RxPacket_partial_5, 
    };
    
    for (auto pkt : pkts) 
    {
        bool ready = false;
        ready = network_io->SBTPacketHandler(pkt, pkt->len, pend_read, sess_index);
        if(ready)
            receiver_to_parser.push(pkt);
    }
    
    for(int i = 0; i < 4; i++)
        parser_dispatch->dispatch();

//      ^ DIFFERENT SCENARIOS FOR OUCH PARSING ^
// ====================================================

    MessageWithVenue<MessageTypes_t> msgWvenue;
   
    // OrderAccepted Message
    parser_to_store.pop(msgWvenue); 

    auto variant_msg = std::get<BIST::OUCHMessage>(msgWvenue.msg);
    auto* msg1 = std::get<BIST::RX::OUCHOrderAcceptedMessage*>(variant_msg);
    
    ASSERT_NE(msg1, nullptr);
    EXPECT_EQ(msg1->message_type, 'A');
    EXPECT_EQ(msg1->timestamp, 1);
    EXPECT_EQ(msg1->order_book_id, 2);
    EXPECT_EQ(msg1->side, 'B');
    EXPECT_EQ(msg1->order_id, 1);
    EXPECT_EQ(msg1->quantity, 2000);
    EXPECT_EQ(msg1->price, 10000);
    EXPECT_EQ(std::string(msg1->order_token, 14), "TOKEN000000002");
    // default
    EXPECT_EQ(msg1->time_in_force, 0);
    EXPECT_EQ(msg1->open_close, 0);
    EXPECT_EQ(msg1->order_state, 1);
    EXPECT_EQ(msg1->client_category, 0);
    EXPECT_EQ(msg1->offhours, 0);
    EXPECT_EQ(msg1->smp_level, 0);
    EXPECT_EQ(msg1->smp_method, 0);
    EXPECT_EQ(msg1->pre_trade_qty, 0);
    EXPECT_EQ(msg1->display_qty, 0);
    EXPECT_EQ(std::string(msg1->client_account, 16), std::string(16, '\0'));
    EXPECT_EQ(std::string(msg1->customer_info, 15), std::string(15, '\0'));
    EXPECT_EQ(std::string(msg1->exchange_info, 32), std::string(32, '\0'));
    EXPECT_EQ(std::string(msg1->smp_ID, 3), std::string(3, '\0'));

    // Cancelled Message
    parser_to_store.pop(msgWvenue); 

    variant_msg = std::get<BIST::OUCHMessage>(msgWvenue.msg);
    auto* msg2 = std::get<BIST::RX::OUCHOrderCancelledMessage*>(variant_msg);

    ASSERT_NE(msg2, nullptr);
    EXPECT_EQ(msg2->message_type, 'C');
    EXPECT_EQ(msg2->timestamp, 2);
    EXPECT_EQ(msg2->order_book_id, 2);
    EXPECT_EQ(msg2->side, 'B');
    EXPECT_EQ(msg2->order_id, 1);
    EXPECT_EQ(std::string(msg2->order_token, 14), "TOKEN000000002");
    // default
    EXPECT_EQ(msg2->reason, 0);


  // Executed Message
    parser_to_store.pop(msgWvenue); 

    variant_msg = std::get<BIST::OUCHMessage>(msgWvenue.msg);
    auto* msg3 = std::get<BIST::RX::OUCHOrderExecutedMessage*>(variant_msg);

    ASSERT_NE(msg3, nullptr);
    EXPECT_EQ(msg3->message_type, 'E');
    EXPECT_EQ(msg3->timestamp, 3);
    EXPECT_EQ(msg3->order_book_id, 2);
    EXPECT_EQ(msg3->traded_quantity, 1000);
    EXPECT_EQ(msg3->trade_price, 10000);
    EXPECT_EQ(std::string(msg3->order_token, 14), "TOKEN000000002");
    // default
    EXPECT_EQ(msg3->client_category, 0);
    EXPECT_EQ(std::string(msg3->match_id, 12), std::string(12, '\0'));
    EXPECT_EQ(std::string(msg3->reserved, 16), std::string(16, '\0'));

    // Executed Message
    parser_to_store.pop(msgWvenue); 

    variant_msg = std::get<BIST::OUCHMessage>(msgWvenue.msg);
    auto* msg4 = std::get<BIST::RX::OUCHOrderExecutedMessage*>(variant_msg);

    ASSERT_NE(msg4, nullptr);
    EXPECT_EQ(msg4->message_type, 'E');
    EXPECT_EQ(msg4->timestamp, 3);
    EXPECT_EQ(msg4->order_book_id, 2);
    EXPECT_EQ(msg4->traded_quantity, 1000);
    EXPECT_EQ(msg4->trade_price, 10000);
    EXPECT_EQ(std::string(msg4->order_token, 14), "TOKEN000000002");
    // default
    EXPECT_EQ(msg4->client_category, 0);
    EXPECT_EQ(std::string(msg4->match_id, 12), std::string(12, '\0'));
    EXPECT_EQ(std::string(msg4->reserved, 16), std::string(16, '\0'));

    parser_dispatch.reset();
    network_io.reset();
    login.reset();
    builder_fix.reset();
    sbt.reset();
    sess_mngr.reset();
    txPkt_pool.reset();
}

