#pragma once

#include "FIXMessage.h"
#include "GeneratedITCHMessages_BIST.h"
#include "GeneratedITCHMessages_NASDAQ.h"
#include "GeneratedOUCHMessages_BIST.h"
#include "GeneratedOUCHMessages_NASDAQ.h"
#include "MessageWithVenue.h"
#include "NetworkPackets.h"
#include "Order.h"

#include <absl/hash/hash.h>

#include <vector>
#include <cstdint>
#include <array>
#include <chrono>

namespace test_data_network {
//===========================================================================
//                  NETWORK_IO TEST DATA
//===========================================================================
    
    // FIX Login Accepted - BIST FIX (session 0)
    inline const std::vector<uint8_t> fix_login_ack = {
        '8','=','F','I','X','T','.','1','.','1','\x01',  // 8  – BeginString
        '9','=','1','9','\x01',                          // 9  – BodyLength
        '3','5','=','A','\x01',                          // 35 – MsgType = A
        '3','4','=','1','\x01', 
        '9','8','=','0','\x01',                          // 98 – EncryptMethod 
        '1','0','8','=','3','0','\x01',                  // 108– HeartBtInt   
        '1','0','=','2','3','1','\x01',                  // 10 – CheckSum
    };

    // SBT Login Accepted - BIST OUCH (session 1)
    inline const std::vector<uint8_t> sbt_login_accepted_bist = {
        0x00, 0x1F,                                     // packet_length = 31
        'A',                                            // packet_type
        ' ',' ',' ',' ',' ',' ','B','I','S','T',        // session[10] = "BIST"
        ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
        ' ',' ',' ',' ',' ',' ',' ',' ',' ','1'         // sequence_number[20] = "1"
    };

    // SBT Login Accepted - NASDAQ OUCH (session 2)
    inline const std::vector<uint8_t> sbt_login_accepted_nq = {
        0x00, 0x1F,
        'A',
        ' ',' ',' ',' ',' ',' ',' ','N','Q','1',       
        ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
        ' ',' ',' ',' ',' ',' ',' ',' ',' ','1'       
    };

    // ==================== ORDER ACK FROM EXCHANGE ======================= 
    
    inline const std::vector<uint8_t> fix_order_ack = {
        '8','=','F','I','X','.','4','.','2','\x01',
        '9','=','1','0','2','\x01',
        '3','5','=','8','\x01',
        '3','4','=','1','\x01',
        '4','9','=','B','I','S','T','\x01',
        '5','6','=','C','L','I','E','N','T','0','1','\x01',
        '1','1','=','C','L','I','E','N','T','0','0','0','0','0','0','1','\x01',
        '3','7','=','O','R','D','0','0','0','0','1','\x01',
        '5','5','=','G','A','R','A','N','\x01',
        '5','4','=','1','\x01',
        '3','8','=','1','0','0','\x01',
        '4','4','=','1','2','.','3','4','5','6','\x01',
        '3','9','=','0','\x01',
        '1','5','0','=','0','\x01',
        '1','0','=','1','8','7','\x01',
    };

     inline const std::vector<uint8_t> fix_order_ack2 = {
        '8','=','F','I','X','.','4','.','2','\x01',
        '9','=','1','1','5','\x01',
        '3','5','=','8','\x01',
        '3','4','=','1','\x01',
        '4','9','=','B','I','S','T','\x01',
        '5','6','=','C','L','I','E','N','T','0','1','\x01',
        '1','1','=','C','L','I','E','N','T','0','0','0','0','0','0','1','\x01',
        '1','7','=','E','X','E','C','0','0','0','0','2','\x01',
        '3','7','=','O','R','D','0','0','0','0','1','\x01',
        '5','5','=','G','A','R','A','N','\x01',
        '5','4','=','1','\x01',
        '3','8','=','1','0','0','\x01',
        '4','4','=','1','2','.','3','4','5','6','\x01',
        '3','9','=','0','\x01',
        '1','5','0','=','0','\x01',
        '1','0','=','1','2','0','\x01',
    };

    inline const std::vector<uint8_t> ouch_bist_order_ack = {
        0x00, 0x8A,                                              // sbt packet_length = 138
        'S',                                                     // sbt packet_type = seqdata
        'A',                                                     // message_type | offset: 0
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,                 // timestamp | offset: 1
        'C','L','I','E','N','T','0','0','0','0','0','0','5',' ', // order_token | offset: 9, len: 14
        0x00,0x00,0x00,0x03,                                     // order_book_id = instrument_id=3 | offset: 23
        'B',                                                     // side = Buy | offset: 27
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,                 // order_id | offset: 28
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x64,                 // quantity = 100 | offset: 36
        0x00,0x00,0x27,0x10,                                     // price = 10000 | offset: 44
        0x00,                                                    // time_in_force = DAY | offset: 48
        0x00,                                                    // open_close | offset: 49
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,                         // client_account | offset: 50, len: 16
        0x01,                                                    // order_state = live | offset: 66
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,                           // customer_info | offset: 67, len: 15
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,                         // exchange_info | offset: 82, len: 32
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,                 // pre_trade_qty | offset: 114
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,                 // display_qty | offset: 122
        0x00,                                                    // client_category | offset: 130
        0x00,                                                    // offhours | offset: 131
        0x00,                                                    // smp_level | offset: 132
        0x00,                                                    // smp_method | offset: 133
        0x00,0x00,0x00,                                          // smp_id | offset: 134
    };
   
    inline const std::vector<uint8_t> ouch_nq_order_ack = {
        0x00, 0x41,                                              // sbt packet_length = 65
        'S',                                                     // sbt packet_type = seqdata
        'A',                                                     // message_type | offset: 0
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,                 // timestamp | offset: 1
        0x00,0x00,0x00,0x01,                                     // user_ref_num | offset: 9
        'S',                                                     // side = Sell | offset: 13
        0x00,0x00,0x00,0x64,                                     // quantity = 100 | offset: 14
        'A','A','P','L',' ',' ',' ',' ',                         // symbol | offset: 18
        0x00,0x00,0x00,0x00,0x00,0x00,0x27,0x10,                 // price = 10000 | offset: 26
        '0',                                                     // time_in_force = DAY | offset: 34
        'Y',                                                     // display | offset: 35
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,                 // order_reference_number | offset: 36
        'A',                                                     // capacity | offset: 44
        'N',                                                     // intermarket_sweep_eligibility | offset: 45
        'N',                                                     // cross_type | offset: 46
        'L',                                                     // order_state | offset: 47
        'C','L','I','E','N','T','0','0','0','0','0','0','8',' ', // cl_ord_id | offset: 48
        0x00,0x00,                                               // appendage_length | offset: 62
    };

    inline const std::vector<uint8_t> itch_bist_add_order = {
        'A',                                        // message_type | offset: 0
        0x00,0x00,0x00,0x01,                        // timestamp_ns | offset: 1
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,    // order_id | offset: 5
        0x00,0x00,0x00,0x03,                        // order_book_id | offset: 13
        'B',                                        // side | offset: 17
        0x00,0x00,0x00,0x01,                        // ranking_seq_number | offset: 18
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x64,    // quantity = 100 | offset: 22
        0x00,0x00,0x27,0x10,                        // price = 10000 | offset: 30 (SIGNED)
        0x00,0x00,                                  // order_attributes | offset: 34
        0x02,                                       // lot_type = Round Lot | offset: 36
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,    // ranking_time | offset: 37
    };

    inline const std::vector<uint8_t> itch_bist_add_order2 = {
        'A',                                        // message_type | offset: 0
        0x00,0x00,0x00,0x01,                        // timestamp_ns | offset: 1
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,    // order_id | offset: 5
        0x00,0x00,0x00,0x03,                        // order_book_id | offset: 13
        'B',                                        // side | offset: 17
        0x00,0x00,0x00,0x01,                        // ranking_seq_number | offset: 18
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x65,    // quantity = 101 | offset: 22
        0x00,0x00,0x27,0x10,                        // price = 10000 | offset: 30 (SIGNED)
        0x00,0x00,                                  // order_attributes | offset: 34
        0x02,                                       // lot_type = Round Lot | offset: 36
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,    // ranking_time | offset: 37
    };

    inline const std::vector<uint8_t> itch_nq_add_order = {
        'A',                                        // message_type | offset: 0
        0x00,0x01,                                  // stock_locate | offset: 1
        0x00,0x01,                                  // tracking_number | offset: 3
        0x00,0x00,0x00,0x00,0x00,0x01,              // timestamp = 1ns | offset: 5
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,    // order_ref | offset: 11
        'S',                                        // side = Sell | offset: 19
        0x00,0x00,0x00,0x64,                        // shares = 100 | offset: 20
        'A','A','P','L',' ',' ',' ',' ',            // stock | offset: 24
        0x05,0xF5,0xE1,0x00,                        // price = 100000000 (10000 * 10000) | offset: 32
    };
}