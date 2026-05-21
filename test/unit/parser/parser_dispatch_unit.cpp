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

TEST(ParserDispatchTest, MixedProtocolTraffic)
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
    auto* sess_state = sess_mngr->getSessionState(sess_index);
    auto& seq_fix = sess_state->fix;
    
    RxPacket* rxPkt;
    PendingQueue<RxPacket *, 256> pend_read;

// ====================================================
//       v TRAFFIC FOR PARSING DISPATCH v 
    
    std::vector<RxPacket*> pkts = {
        &test_data_parser::ouch_bist_RxPacket_partial_1, 
        &test_data_parser::ouch_bist_RxPacket_partial_2,
        &test_data_parser::itch_bist_RxPacket_single_1,
        &test_data_parser::fix_RxPacket_partial_1, 
        &test_data_parser::fix_RxPacket_partial_2,
        &test_data_parser::ouch_bist_RxPacket_partial_3,
        &test_data_parser::ouch_bist_RxPacket_partial_4,  
        &test_data_parser::fix_RxPacket_partial_3,
        &test_data_parser::fix_RxPacket_partial_4,
        &test_data_parser::ouch_bist_RxPacket_partial_5,
        &test_data_parser::itch_nasdaq_RxPacket_single_1,  
        &test_data_parser::fix_RxPacket_partial_5, 
        &test_data_parser::fix_RxPacket_partial_6,   
    };
    
    for (auto pkt : pkts) 
    {

        if(pkt->protocol == Protocol::OUCH) 
        {
            bool ouch_rxPkt_ready = false; 
            ouch_rxPkt_ready = network_io->SBTPacketHandler(pkt, pkt->len, pend_read, sess_index);
            if(ouch_rxPkt_ready)
                receiver_to_parser.push(pkt);
        }
        else
        {
            receiver_to_parser.push(pkt);
        }
    }

    for(int i = 0; i < 12; i++)
        parser_dispatch->dispatch();

//      ^ TRAFFIC FOR PARSING DISPATCH ^
// ====================================================
    
    MessageWithVenue<MessageTypes_t> msgWvenue;
   
    // OUCH OrderAccepted Message
    parser_to_store.pop(msgWvenue); 

    auto variant_msg_ouch = std::get<BIST::OUCHMessage>(msgWvenue.msg);
    auto* msg1 = std::get<BIST::RX::OUCHOrderAcceptedMessage*>(variant_msg_ouch);
    
    ASSERT_NE(msg1, nullptr);
    EXPECT_EQ(msg1->message_type, 'A');
    EXPECT_EQ(msg1->timestamp, 1);
    EXPECT_EQ(msg1->order_book_id, 2);
    EXPECT_EQ(msg1->side, 'B');
    EXPECT_EQ(msg1->order_id, 1);
    EXPECT_EQ(msg1->quantity, 2000);
    EXPECT_EQ(msg1->price, 10000);
    EXPECT_EQ(std::string(msg1->order_token, 14), "TOKEN000000002");
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
 
    
    // ITCH OrderBookDirectory Message
    parser_to_store.pop(msgWvenue); 

    auto variant_msg_itch_bist = std::get<BIST::ITCHMessage>(msgWvenue.msg);
    auto* msg2 = std::get<BIST::ITCHOrderBookDirectoryMessage*>(variant_msg_itch_bist);

    ASSERT_NE(msg2, nullptr);
    EXPECT_EQ(msg2->message_type, 'R');
    EXPECT_EQ(msg2->timestamp_ns, 48);
    EXPECT_EQ(msg2->order_book_id, 3);
    EXPECT_EQ(msg2->financial_product, 5);
    EXPECT_EQ(msg2->decimals_in_price, 2);
    EXPECT_EQ(msg2->decimals_in_nominal, 2);
    EXPECT_EQ(msg2->odd_lot_size, 1);
    EXPECT_EQ(msg2->round_lot_size, 1);
    EXPECT_EQ(msg2->block_lot_size, 1);
    EXPECT_EQ(msg2->nominal_value, 0);
    EXPECT_EQ(msg2->number_of_legs, 0);
    EXPECT_EQ(msg2->underlying_order_book_id, 0);
    EXPECT_EQ(msg2->strike_price, 0);
    EXPECT_EQ(msg2->expiration_date, 0);
    EXPECT_EQ(msg2->decimals_in_strike_price, 2);
    EXPECT_EQ(msg2->put_or_call, 0);
    EXPECT_EQ(msg2->ranking_type, 1);
    EXPECT_STREQ(msg2->trading_currency, "TRY");
    EXPECT_EQ(msg2->symbol[0], 'G');
    EXPECT_EQ(msg2->symbol[1], 'A');
    EXPECT_EQ(msg2->symbol[2], 'R');
    EXPECT_EQ(msg2->symbol[3], 'A');
    EXPECT_EQ(msg2->symbol[4], 'N');
    EXPECT_EQ(msg2->isin[0], 'T');
    EXPECT_EQ(msg2->isin[1], 'R');
    EXPECT_EQ(msg2->isin[2], 'G');

     // FIX New Order (D)
    parser_to_store.pop(msgWvenue); 

    auto* msg3 = std::get<FIXMessage*>(msgWvenue.msg);

    ASSERT_NE(msg3, nullptr);
    EXPECT_EQ(msg3->msg_type, 'D');
    EXPECT_EQ(msg3->seqnum, 1);
    EXPECT_EQ(msg3->quantity, 100);
    EXPECT_EQ(msg3->price, 10000);
    EXPECT_EQ(msg3->side, 1);
    EXPECT_STREQ(msg3->symbol, "GARAN");
    EXPECT_STREQ(msg3->cl_ord_id, "O1");
    EXPECT_STREQ(msg3->order_id, "");
    EXPECT_STREQ(msg3->exec_id, "");
    EXPECT_STREQ(msg3->orig_cl_ord_id, "");
    EXPECT_EQ(msg3->last_qty, 0);
    EXPECT_EQ(msg3->last_price, 0);
    EXPECT_EQ(msg3->leaves_qty, 0);
    EXPECT_EQ(msg3->filled_qty, 0);
    EXPECT_EQ(msg3->instrument_id, 0);
    EXPECT_EQ(msg3->exec_type, 0);
    EXPECT_EQ(msg3->ord_status, 0);
    EXPECT_EQ(msg3->ord_type, 0);
    EXPECT_EQ(msg3->time_in_force, 0);
    EXPECT_EQ(msg3->cxl_rej_response_to, 0);
    EXPECT_EQ(msg3->cxl_rej_reason, 255);
    
    // FIX Cancel (F)
    parser_to_store.pop(msgWvenue);

    auto* msg4 = std::get<FIXMessage*>(msgWvenue.msg);

    ASSERT_NE(msg4, nullptr);
    EXPECT_EQ(msg4->msg_type, 'F');
    EXPECT_EQ(msg4->seqnum, 2);
    EXPECT_STREQ(msg4->symbol, "TUPRS");
    EXPECT_STREQ(msg4->cl_ord_id, "O2");
    EXPECT_STREQ(msg4->orig_cl_ord_id, "O1");
    EXPECT_EQ(msg4->price, 0);
    EXPECT_EQ(msg4->side, 0);
    EXPECT_STREQ(msg4->order_id, "");
    EXPECT_STREQ(msg4->exec_id, "");
    EXPECT_EQ(msg4->last_qty, 0);
    EXPECT_EQ(msg4->last_price, 0);
    EXPECT_EQ(msg4->leaves_qty, 0);
    EXPECT_EQ(msg4->filled_qty, 0);
    EXPECT_EQ(msg4->instrument_id, 0);
    EXPECT_EQ(msg4->exec_type, 0);
    EXPECT_EQ(msg4->ord_status, 0);
    EXPECT_EQ(msg4->ord_type, 0);
    EXPECT_EQ(msg4->time_in_force, 0);
    EXPECT_EQ(msg4->cxl_rej_response_to, 0);
    EXPECT_EQ(msg4->cxl_rej_reason, 255);

    // OUCH Cancelled Message
    parser_to_store.pop(msgWvenue); 

    variant_msg_ouch = std::get<BIST::OUCHMessage>(msgWvenue.msg);
    auto* msg5 = std::get<BIST::RX::OUCHOrderCancelledMessage*>(variant_msg_ouch);

    ASSERT_NE(msg5, nullptr);
    EXPECT_EQ(msg5->message_type, 'C');
    EXPECT_EQ(msg5->timestamp, 2);
    EXPECT_EQ(msg5->order_book_id, 2);
    EXPECT_EQ(msg5->side, 'B');
    EXPECT_EQ(msg5->order_id, 1);
    EXPECT_EQ(std::string(msg5->order_token, 14), "TOKEN000000002");
    EXPECT_EQ(msg5->reason, 0);


  // OUCH Executed Message
    parser_to_store.pop(msgWvenue); 

    variant_msg_ouch = std::get<BIST::OUCHMessage>(msgWvenue.msg);
    auto* msg6 = std::get<BIST::RX::OUCHOrderExecutedMessage*>(variant_msg_ouch);

    ASSERT_NE(msg6, nullptr);
    EXPECT_EQ(msg6->message_type, 'E');
    EXPECT_EQ(msg6->timestamp, 3);
    EXPECT_EQ(msg6->order_book_id, 2);
    EXPECT_EQ(msg6->traded_quantity, 1000);
    EXPECT_EQ(msg6->trade_price, 10000);
    EXPECT_EQ(std::string(msg6->order_token, 14), "TOKEN000000002");
    EXPECT_EQ(msg6->client_category, 0);
    EXPECT_EQ(std::string(msg6->match_id, 12), std::string(12, '\0'));
    EXPECT_EQ(std::string(msg6->reserved, 16), std::string(16, '\0'));

    // FIX Execution Report (8)
    parser_to_store.pop(msgWvenue);

    auto* msg7 = std::get<FIXMessage*>(msgWvenue.msg);

    ASSERT_NE(msg7, nullptr);
    EXPECT_EQ(msg7->msg_type, '8');
    EXPECT_EQ(msg7->seqnum, 3);
    EXPECT_EQ(msg7->last_qty, 100);       
    EXPECT_EQ(msg7->last_price, 10000);    
    EXPECT_STREQ(msg7->cl_ord_id, "O1");
    EXPECT_EQ(msg7->ord_status, '2');       
    EXPECT_EQ(msg7->exec_type, 'F');      
    EXPECT_EQ(msg7->quantity, 0);
    EXPECT_EQ(msg7->price, 0);
    EXPECT_EQ(msg7->side, 0);
    EXPECT_STREQ(msg7->symbol, "");
    EXPECT_STREQ(msg7->order_id, "");
    EXPECT_STREQ(msg7->exec_id, "");
    EXPECT_STREQ(msg7->orig_cl_ord_id, "");
    EXPECT_EQ(msg7->leaves_qty, 0);
    EXPECT_EQ(msg7->filled_qty, 0);
    EXPECT_EQ(msg7->instrument_id, 0);
    EXPECT_EQ(msg7->ord_type, 0);
    EXPECT_EQ(msg7->time_in_force, 0);
    EXPECT_EQ(msg7->cxl_rej_response_to, 0);
    EXPECT_EQ(msg7->cxl_rej_reason, 255);

    // OUCH Executed Message
    parser_to_store.pop(msgWvenue); 

    variant_msg_ouch = std::get<BIST::OUCHMessage>(msgWvenue.msg);
    auto* msg8 = std::get<BIST::RX::OUCHOrderExecutedMessage*>(variant_msg_ouch);

    ASSERT_NE(msg8, nullptr);
    EXPECT_EQ(msg8->message_type, 'E');
    EXPECT_EQ(msg8->timestamp, 3);
    EXPECT_EQ(msg8->order_book_id, 2);
    EXPECT_EQ(msg8->traded_quantity, 1000);
    EXPECT_EQ(msg8->trade_price, 10000);
    EXPECT_EQ(std::string(msg8->order_token, 14), "TOKEN000000002");
    EXPECT_EQ(msg8->client_category, 0);
    EXPECT_EQ(std::string(msg8->match_id, 12), std::string(12, '\0'));
    EXPECT_EQ(std::string(msg8->reserved, 16), std::string(16, '\0'));

    // ITCH StockDirectory Message
    parser_to_store.pop(msgWvenue); 

    auto variant_msg_itch_nasdaq = std::get<NASDAQ::ITCHMessage>(msgWvenue.msg);
    auto* msg9 = std::get<NASDAQ::ITCHStockDirectoryMessage*>(variant_msg_itch_nasdaq);
    
    ASSERT_NE(msg9, nullptr);
    EXPECT_EQ(msg9->message_type, 'R');
    EXPECT_EQ(msg9->stock_locate, 1);
    EXPECT_EQ(msg9->tracking_number, 2);
    EXPECT_EQ(msg9->timestamp, 48);
    EXPECT_EQ(msg9->round_lot_size, 100);
    EXPECT_EQ(msg9->market_category, 'Q');
    EXPECT_EQ(msg9->financial_status_indicator, 'N');
    EXPECT_EQ(msg9->round_lots_only, 'N');
    EXPECT_EQ(msg9->issue_classification, 'C');
    EXPECT_EQ(msg9->issue_sub_type[0], 'Z');
    EXPECT_EQ(msg9->issue_sub_type[1], 'Z');
    EXPECT_EQ(msg9->authenticity, 'P');
    EXPECT_EQ(msg9->short_sale_threshold_indicator, 'N');
    EXPECT_EQ(msg9->ipo_flag, 'N');
    EXPECT_EQ(msg9->luld_reference_price_tier, '1');
    EXPECT_EQ(msg9->etp_flag, 'N');
    EXPECT_EQ(msg9->etp_leverage_factor, 1);
    EXPECT_EQ(msg9->inverse_indicator, 'N');
    EXPECT_EQ(msg9->stock[0], 'A');
    EXPECT_EQ(msg9->stock[1], 'A');
    EXPECT_EQ(msg9->stock[2], 'P');
    EXPECT_EQ(msg9->stock[3], 'L');

    // FIX Execution Report (8)
    parser_to_store.pop(msgWvenue); 

    auto* msg10 = std::get<FIXMessage*>(msgWvenue.msg);

    ASSERT_NE(msg10, nullptr);
    EXPECT_EQ(msg10->msg_type, '8');
    EXPECT_EQ(msg10->seqnum, 4);
    EXPECT_EQ(msg10->last_qty, 100);       
    EXPECT_EQ(msg10->last_price, 10000);    
    EXPECT_STREQ(msg10->cl_ord_id, "O1");
    EXPECT_EQ(msg10->ord_status, '2');      
    EXPECT_EQ(msg10->exec_type, 'F');      
    EXPECT_EQ(msg10->quantity, 0);
    EXPECT_EQ(msg10->price, 0);
    EXPECT_EQ(msg10->side, 0);
    EXPECT_STREQ(msg10->symbol, "");
    EXPECT_STREQ(msg10->order_id, "");
    EXPECT_STREQ(msg10->exec_id, "");
    EXPECT_STREQ(msg10->orig_cl_ord_id, "");
    EXPECT_EQ(msg10->leaves_qty, 0);
    EXPECT_EQ(msg10->filled_qty, 0);
    EXPECT_EQ(msg10->instrument_id, 0);
    EXPECT_EQ(msg10->ord_type, 0);
    EXPECT_EQ(msg10->time_in_force, 0);
    EXPECT_EQ(msg10->cxl_rej_response_to, 0);
    EXPECT_EQ(msg10->cxl_rej_reason, 255);

    // FIX  New Order (D)
    parser_to_store.pop(msgWvenue);

    auto* msg11 = std::get<FIXMessage*>(msgWvenue.msg);

    ASSERT_NE(msg11, nullptr);
    EXPECT_EQ(msg11->msg_type, 'D');
    EXPECT_EQ(msg11->seqnum, 5);
    EXPECT_EQ(msg11->quantity, 100);
    EXPECT_EQ(msg11->price, 10000);
    EXPECT_EQ(msg11->side, 1);
    EXPECT_STREQ(msg11->symbol, "GARAN");
    EXPECT_STREQ(msg11->cl_ord_id, "O1");
    EXPECT_STREQ(msg11->order_id, "");
    EXPECT_STREQ(msg11->exec_id, "");
    EXPECT_STREQ(msg11->orig_cl_ord_id, "");
    EXPECT_EQ(msg11->last_qty, 0);
    EXPECT_EQ(msg11->last_price, 0);
    EXPECT_EQ(msg11->leaves_qty, 0);
    EXPECT_EQ(msg11->filled_qty, 0);
    EXPECT_EQ(msg11->instrument_id, 0);
    EXPECT_EQ(msg11->exec_type, 0);
    EXPECT_EQ(msg11->ord_status, 0);
    EXPECT_EQ(msg11->ord_type, 0);
    EXPECT_EQ(msg11->time_in_force, 0);
    EXPECT_EQ(msg11->cxl_rej_response_to, 0);
    EXPECT_EQ(msg11->cxl_rej_reason, 255);
   
    // FIX Cancel (F)
    parser_to_store.pop(msgWvenue);

    auto* msg12 = std::get<FIXMessage*>(msgWvenue.msg);

    ASSERT_NE(msg12, nullptr);
    EXPECT_EQ(msg12->msg_type, 'F');
    EXPECT_EQ(msg12->seqnum, 6);
    EXPECT_STREQ(msg12->symbol, "TUPRS");
    EXPECT_STREQ(msg12->cl_ord_id, "O2");
    EXPECT_STREQ(msg12->orig_cl_ord_id, "O1");
    EXPECT_EQ(msg12->price, 0);
    EXPECT_EQ(msg12->side, 0);
    EXPECT_STREQ(msg12->order_id, "");
    EXPECT_STREQ(msg12->exec_id, "");
    EXPECT_EQ(msg12->last_qty, 0);
    EXPECT_EQ(msg12->last_price, 0);
    EXPECT_EQ(msg12->leaves_qty, 0);
    EXPECT_EQ(msg12->filled_qty, 0);
    EXPECT_EQ(msg12->instrument_id, 0);
    EXPECT_EQ(msg12->exec_type, 0);
    EXPECT_EQ(msg12->ord_status, 0);
    EXPECT_EQ(msg12->ord_type, 0);
    EXPECT_EQ(msg12->time_in_force, 0);
    EXPECT_EQ(msg12->cxl_rej_response_to, 0);
    EXPECT_EQ(msg12->cxl_rej_reason, 255);

}

