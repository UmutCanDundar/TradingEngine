#include <gtest/gtest.h>

#include "Parser_Dispatch.h"
#include "SessionManager.h"
#include "LoginController.h"
#include "SoupBinTcp.h"
#include "Builder_FIX.h"
#include "NetworkIO.h"
#include "dataset.h"

#include <memory>

TEST(FixParserDispatchTest, MultFixPkt)
{
    auto inPkt_pool = std::make_unique<InPacketPoolManager>();
    spscFIXInSessionQueue_t parser_to_fixbuilder_in;
    spscOutPacketQueue_t receiver_to_parser;
    spscInPacketQueue_t builder_to_sender;
    spscMessageQueue_t parser_to_store; 
    spscFIXOutSessionQueue_t parser_to_fixbuilder_out;
    spscDbQueue_t db_to_parser; 
    auto sess_mngr     = std::make_unique<SessionManager>();
    auto sbt           = std::make_unique<SoupBinTcp>(*sess_mngr);
    auto builder_fix   = std::make_unique<Builder_FIX>(*sess_mngr);
    auto login         = std::make_unique<LoginController>(*sbt, *builder_fix, *sess_mngr);
    auto network_io    = std::make_unique<NetworkIO>(receiver_to_parser, builder_to_sender, *sess_mngr, *sbt, *login, *inPkt_pool);
    
    auto parser_dispatch = std::make_unique<Parser_Dispatch>(
                                        receiver_to_parser,
                                        parser_to_store,
                                        parser_to_fixbuilder_out,
                                        parser_to_fixbuilder_in,
                                        *sess_mngr,
                                        db_to_parser,
                                        *network_io
                                    );

    uint8_t sess_index = sess_mngr->getSessionIndex(Venue::BIST, Protocol::FIX);
    auto* sess_state = sess_mngr->getSessionState(sess_index);
    auto& seq_fix = sess_state->fix;

// ====================================================
//       v DIFFERENT SCENARIOS FOR FIX PARSING v 


// SCENARIO 1: ONE PACKET WITH THREE COMPLETE FIX MESSAGES IN ORDER 
    OutPacket* pkt1 = &test_data::fix_outpacket_full_1;
    parser_dispatch->parseFIX(pkt1);

    seq_fix.set_expected_seq(1);

// SCENARIO 2: THREE PACKETS CONTAINING PARTIAL FIX MESSAGES IN ORDER
    std::array<OutPacket*, 3> pkts2 = {
        &test_data::fix_outpacket_partial_1, 
        &test_data::fix_outpacket_partial_2, 
        &test_data::fix_outpacket_partial_3 
    };
    for (auto pkt2 : pkts2) 
        parser_dispatch->parseFIX(pkt2);

// SCENARIO 3: THREE PACKETS CONTAINING PARTIAL FIX MESSAGES NOT IN ORDER
    std::array<OutPacket*, 3> pkts3 = {
        &test_data::fix_outpacket_partial_4, 
        &test_data::fix_outpacket_partial_5, 
        &test_data::fix_outpacket_partial_6 
    };
    //seq_fix.set_expected_seq(4);
    
    for (auto pkt3 : pkts3) 
        parser_dispatch->parseFIX(pkt3);

//      ^ DIFFERENT SCENARIOS FOR FIX PARSING ^
// ====================================================

// SCENARIO 1-2 TEST
int scenario = 2;
for(int test = 0; test < scenario; test++) 
{ 
    MessageWithVenue<MessageTypes_t> msgWvenue;
    
    // 1 New Order (D)
    parser_to_store.pop(msgWvenue); 
    FIXMessage* msg = std::get<FIXMessage*>(msgWvenue.msg);

    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->msg_type, 'D');
    EXPECT_EQ(msg->seqnum, 1);
    EXPECT_EQ(msg->quantity, 100);
    EXPECT_EQ(msg->price, 10000);
    EXPECT_EQ(msg->side, 1);
    EXPECT_STREQ(msg->symbol, "GARAN");
    EXPECT_STREQ(msg->cl_ord_id, "O1");
    // default
    EXPECT_STREQ(msg->order_id, "");
    EXPECT_STREQ(msg->exec_id, "");
    EXPECT_STREQ(msg->orig_cl_ord_id, "");
    EXPECT_EQ(msg->last_qty, 0);
    EXPECT_EQ(msg->last_price, 0);
    EXPECT_EQ(msg->leaves_qty, 0);
    EXPECT_EQ(msg->filled_qty, 0);
    EXPECT_EQ(msg->instrument_id, 0);
    EXPECT_EQ(msg->exec_type, 0);
    EXPECT_EQ(msg->ord_status, 0);
    EXPECT_EQ(msg->ord_type, 0);
    EXPECT_EQ(msg->time_in_force, 0);
    EXPECT_EQ(msg->cxl_rej_response_to, 0);
    EXPECT_EQ(msg->cxl_rej_reason, 255);
    
    // 2 Cancel (F)
    parser_to_store.pop(msgWvenue);
    msg = std::get<FIXMessage*>(msgWvenue.msg);

    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->msg_type, 'F');
    EXPECT_EQ(msg->seqnum, 2);
    EXPECT_STREQ(msg->symbol, "TUPRS");
    EXPECT_STREQ(msg->cl_ord_id, "O2");
    EXPECT_STREQ(msg->orig_cl_ord_id, "O1");
    // default
    EXPECT_EQ(msg->price, 0);
    EXPECT_EQ(msg->side, 0);
    EXPECT_STREQ(msg->order_id, "");
    EXPECT_STREQ(msg->exec_id, "");
    EXPECT_EQ(msg->last_qty, 0);
    EXPECT_EQ(msg->last_price, 0);
    EXPECT_EQ(msg->leaves_qty, 0);
    EXPECT_EQ(msg->filled_qty, 0);
    EXPECT_EQ(msg->instrument_id, 0);
    EXPECT_EQ(msg->exec_type, 0);
    EXPECT_EQ(msg->ord_status, 0);
    EXPECT_EQ(msg->ord_type, 0);
    EXPECT_EQ(msg->time_in_force, 0);
    EXPECT_EQ(msg->cxl_rej_response_to, 0);
    EXPECT_EQ(msg->cxl_rej_reason, 255);
    
    // 3 Execution Report (8)
    parser_to_store.pop(msgWvenue);
    msg = std::get<FIXMessage*>(msgWvenue.msg);

    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->msg_type, '8');
    EXPECT_EQ(msg->seqnum, 3);
    EXPECT_EQ(msg->last_qty, 100);       // 32=100
    EXPECT_EQ(msg->last_price, 10000);    // 31=10000
    EXPECT_STREQ(msg->cl_ord_id, "O1");
    EXPECT_EQ(msg->ord_status, '2');       // 39=2
    EXPECT_EQ(msg->exec_type, 'F');      // 150=F
    //default
    EXPECT_EQ(msg->quantity, 0);
    EXPECT_EQ(msg->price, 0);
    EXPECT_EQ(msg->side, 0);
    EXPECT_STREQ(msg->symbol, "");
    EXPECT_STREQ(msg->order_id, "");
    EXPECT_STREQ(msg->exec_id, "");
    EXPECT_STREQ(msg->orig_cl_ord_id, "");
    EXPECT_EQ(msg->leaves_qty, 0);
    EXPECT_EQ(msg->filled_qty, 0);
    EXPECT_EQ(msg->instrument_id, 0);
    EXPECT_EQ(msg->ord_type, 0);
    EXPECT_EQ(msg->time_in_force, 0);
    EXPECT_EQ(msg->cxl_rej_response_to, 0);
    EXPECT_EQ(msg->cxl_rej_reason, 255);
}

// SCENARIO 3 TEST

    MessageWithVenue<MessageTypes_t> msgWvenue;

    // 1 Execution Report (8)
    parser_to_store.pop(msgWvenue); 
    FIXMessage* msg = std::get<FIXMessage*>(msgWvenue.msg);

    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->msg_type, '8');
    EXPECT_EQ(msg->seqnum, 4);
    EXPECT_EQ(msg->last_qty, 100);       // 32=100
    EXPECT_EQ(msg->last_price, 10000);    // 31=10000
    EXPECT_STREQ(msg->cl_ord_id, "O1");
    EXPECT_EQ(msg->ord_status, '2');      // 39=2
    EXPECT_EQ(msg->exec_type, 'F');      // 150=F
    //default
    EXPECT_EQ(msg->quantity, 0);
    EXPECT_EQ(msg->price, 0);
    EXPECT_EQ(msg->side, 0);
    EXPECT_STREQ(msg->symbol, "");
    EXPECT_STREQ(msg->order_id, "");
    EXPECT_STREQ(msg->exec_id, "");
    EXPECT_STREQ(msg->orig_cl_ord_id, "");
    EXPECT_EQ(msg->leaves_qty, 0);
    EXPECT_EQ(msg->filled_qty, 0);
    EXPECT_EQ(msg->instrument_id, 0);
    EXPECT_EQ(msg->ord_type, 0);
    EXPECT_EQ(msg->time_in_force, 0);
    EXPECT_EQ(msg->cxl_rej_response_to, 0);
    EXPECT_EQ(msg->cxl_rej_reason, 255);

    // 2  New Order (D)
    MessageWithVenue<MessageTypes_t> msgWvenue2;
    parser_to_store.pop(msgWvenue2);
    msg = std::get<FIXMessage*>(msgWvenue2.msg);

    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->msg_type, 'D');
    EXPECT_EQ(msg->seqnum, 5);
    EXPECT_EQ(msg->quantity, 100);
    EXPECT_EQ(msg->price, 10000);
    EXPECT_EQ(msg->side, 1);
    EXPECT_STREQ(msg->symbol, "GARAN");
    EXPECT_STREQ(msg->cl_ord_id, "O1");
    // default
    EXPECT_STREQ(msg->order_id, "");
    EXPECT_STREQ(msg->exec_id, "");
    EXPECT_STREQ(msg->orig_cl_ord_id, "");
    EXPECT_EQ(msg->last_qty, 0);
    EXPECT_EQ(msg->last_price, 0);
    EXPECT_EQ(msg->leaves_qty, 0);
    EXPECT_EQ(msg->filled_qty, 0);
    EXPECT_EQ(msg->instrument_id, 0);
    EXPECT_EQ(msg->exec_type, 0);
    EXPECT_EQ(msg->ord_status, 0);
    EXPECT_EQ(msg->ord_type, 0);
    EXPECT_EQ(msg->time_in_force, 0);
    EXPECT_EQ(msg->cxl_rej_response_to, 0);
    EXPECT_EQ(msg->cxl_rej_reason, 255);
   
    // 3 Cancel (F)
    MessageWithVenue<MessageTypes_t> msgWvenue3;
    parser_to_store.pop(msgWvenue3);
    msg = std::get<FIXMessage*>(msgWvenue3.msg);

    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->msg_type, 'F');
    EXPECT_EQ(msg->seqnum, 6);
    EXPECT_STREQ(msg->symbol, "TUPRS");
    EXPECT_STREQ(msg->cl_ord_id, "O2");
    EXPECT_STREQ(msg->orig_cl_ord_id, "O1");
    // default
    EXPECT_EQ(msg->price, 0);
    EXPECT_EQ(msg->side, 0);
    EXPECT_STREQ(msg->order_id, "");
    EXPECT_STREQ(msg->exec_id, "");
    EXPECT_EQ(msg->last_qty, 0);
    EXPECT_EQ(msg->last_price, 0);
    EXPECT_EQ(msg->leaves_qty, 0);
    EXPECT_EQ(msg->filled_qty, 0);
    EXPECT_EQ(msg->instrument_id, 0);
    EXPECT_EQ(msg->exec_type, 0);
    EXPECT_EQ(msg->ord_status, 0);
    EXPECT_EQ(msg->ord_type, 0);
    EXPECT_EQ(msg->time_in_force, 0);
    EXPECT_EQ(msg->cxl_rej_response_to, 0);
    EXPECT_EQ(msg->cxl_rej_reason, 255);

    parser_dispatch.reset();
    network_io.reset();
    login.reset();
    builder_fix.reset();
    sbt.reset();
    sess_mngr.reset();
    inPkt_pool.reset();
}

