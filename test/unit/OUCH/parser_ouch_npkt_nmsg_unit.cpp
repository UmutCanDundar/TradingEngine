#include <gtest/gtest.h>

#include "Parser_Dispatch.h"
#include "SessionManager.h"
#include "LoginController.h"
#include "SoupBinTcp.h"
#include "Builder_FIX.h"
#include "NetworkIO.h"
#include "dataset.h"

#include <memory>

TEST(OuchParserDispatchTest, MultOuchPkt)
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
    auto network_io    = std::make_unique<NetworkIO>(receiver_to_parser, builder_to_sender, *sbt, *sess_mngr, *login, *inPkt_pool);
    
    auto parser_dispatch = std::make_unique<Parser_Dispatch>(
                                        receiver_to_parser,
                                        parser_to_store,
                                        parser_to_fixbuilder_out,
                                        parser_to_fixbuilder_in,
                                        *sess_mngr,
                                        db_to_parser,
                                        *network_io
                                    );


// ====================================================
//       v DIFFERENT SCENARIOS FOR OUCH PARSING v 


// SCENARIO 1: THREE PACKETS CONTAINING PARTIAL OUCH MESSAGES IN ORDER
    std::array<OutPacket*, 3> pkts2 = {
        &test_data::ouch_outpacket_partial_1, 
        &test_data::ouch_outpacket_partial_2, 
        &test_data::ouch_outpacket_partial_3 
    };
    for (auto pkt2 : pkts2) 
        parser_dispatch->parseOUCH_BIST(pkt2);


// SCENARIO 2: THREE PACKETS CONTAINING PARTIAL OUCH MESSAGES NOT IN ORDER
    std::array<OutPacket*, 3> pkts3 = {
        &test_data::ouch_outpacket_partial_4, 
        &test_data::ouch_outpacket_partial_5, 
        &test_data::ouch_outpacket_partial_6 
    };
    for (auto pkt3 : pkts3) 
        parser_dispatch->parseOUCH_BIST(pkt3);

//      ^ DIFFERENT SCENARIOS FOR OUCH PARSING ^
// ====================================================


    parser_dispatch.reset();
    network_io.reset();
    login.reset();
    builder_fix.reset();
    sbt.reset();
    sess_mngr.reset();
    inPkt_pool.reset();
}

