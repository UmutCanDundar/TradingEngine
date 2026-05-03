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

namespace test_data {

//===========================================================================
//                  PARSER TEST DATA
//===========================================================================

    //============= FIX DATA ==================

    inline const std::vector<uint8_t> single_fix_pkt1 = {
        // -------- New Order (D) PARTIAL --------
        '8','=','F','I','X','.','4','.','2','\x01',
        '9','=','0','6','3','\x01',          
        '3','5','=','D','\x01',
        '3','4','=','1','\x01',             
        '4','9','=','C','L','I','E','N','T','\x01',
        '5','6','=','E','X','C','H','\x01',
        '1','1','=','O','1','\x01',          
        '5','5','=','G','A','R','A','N','\x01',
        '3','8','=','1','0','0','\x01',      
        '4','4','=','1','.','0','0','0','\x01', 
        '5','4','=','1','\x01',              
        '1','0','=','1','6','4','\x01',      
    };
    inline const std::vector<uint8_t> single_fix_pkt2 = {
        // -------- Execution Report --------
        '8','=','F','I','X','.','4','.','2','\x01',
        '9','=','0','4','2','\x01',
        '3','5','=','8','\x01',
        '3','4','=','3','\x01',
        '3','9','=','2','\x01',              
        '1','5','0','=','F','\x01',          
        '1','1','=','O','1','\x01',
        '3','2','=','1','0','0','\x01',      
        '3','1','=','1','.','0','0','0','\x01',  
        '1','0','=','0','9','5','\x01',      
    };

    OutPacket fix_outpacket_full_1 = []() {
        OutPacket pkt{};

        const uint8_t data[] = {
            // ── New Order (D) ── offset: 0
            '8','=','F','I','X','.','4','.','2','\x01',
            '9','=','0','6','4','\x01',
            '3','5','=','D','\x01',
            '3','4','=','1','\x01',
            '4','9','=','C','L','I','E','N','T','\x01',
            '5','6','=','E','X','C','H','\x01',
            '1','1','=','O','1','\x01',
            '5','5','=','G','A','R','A','N','\x01',
            '3','8','=','1','0','0','\x01',
            '4','4','=','1','.','0','0','0','\x01',
            '5','4','=','1','\x01',
            '1','0','=','1','6','4','\x01',
            // ── Cancel (F) ── offset: 86
            '8','=','F','I','X','.','4','.','2','\x01',
            '9','=','0','3','1','\x01',
            '3','5','=','F','\x01',
            '3','4','=','2','\x01',
            '1','1','=','O','2','\x01',
            '4','1','=','O','1','\x01',
            '5','5','=','T','U','P','R','S','\x01',
            '1','0','=','0','7','1','\x01',
            // ── Execution Report (8) ── offset: 140
            '8','=','F','I','X','.','4','.','2','\x01',
            '9','=','0','4','3','\x01',
            '3','5','=','8','\x01',
            '3','4','=','3','\x01',
            '3','9','=','2','\x01',
            '1','5','0','=','F','\x01',
            '1','1','=','O','1','\x01',
            '3','2','=','1','0','0','\x01',
            '3','1','=','1','.','0','0','0','\x01',
            '1','0','=','0','9','5','\x01',
        };

        std::memcpy(pkt.data.data(), data, sizeof(data));
        pkt.len        = 207;
        pkt.venue      = Venue::BIST;
        pkt.protocol   = Protocol::FIX;

        return pkt;
    }();

    OutPacket fix_outpacket_partial_1 = []() {
        OutPacket pkt{};

        const uint8_t data[] = {
            // ── New Order (D) ── offset: 0
            '8','=','F','I','X','.','4','.','2','\x01',
            '9','=','0','6','4','\x01',
            '3','5','=','D','\x01',
            '3','4','=','1','\x01',
            '4','9','=','C','L','I','E','N','T','\x01',
            '5','6','=','E','X','C','H','\x01',
            '1','1','=','O','1','\x01',
            '5','5','=','G','A','R','A','N','\x01',
            '3','8','=','1','0','0','\x01',
            '4','4','=','1','.','0','0','0','\x01',
            '5','4','=','1','\x01',
            '1','0','=','1',
        };

        std::memcpy(pkt.data.data(), data, sizeof(data));
        pkt.len        = 84;
        pkt.venue      = Venue::BIST;
        pkt.protocol   = Protocol::FIX;

        return pkt;
    }();
    OutPacket fix_outpacket_partial_2 = []() {
        OutPacket pkt{};

        const uint8_t data[] = {
            '6','4','\x01',
            // ── Cancel (F) ── offset: 87
            '8','=','F','I','X','.','4','.','2','\x01',
            '9','=','0','3','1','\x01',
            '3','5','=','F','\x01',
            '3','4','=','2','\x01',
            '1','1','=','O','2','\x01',
            '4','1','=','O','1','\x01',
            '5','5','=','T','U','P','R','S','\x01',
            '1','0','=','0','7','1','\x01',
            // ── Execution Report (8) ── offset: 141
            '8','=','F','I','X','.','4','.','2','\x01',
            '9','=','0','4','3','\x01',
        };

        std::memcpy(pkt.data.data(), data, sizeof(data));
        pkt.len        = 73;
        pkt.venue      = Venue::BIST;
        pkt.protocol   = Protocol::FIX;

        return pkt;
    }();
    OutPacket fix_outpacket_partial_3 = []() {
        OutPacket pkt{};

        const uint8_t data[] = {
            '3','5','=','8','\x01',
            '3','4','=','3','\x01',
            '3','9','=','2','\x01',
            '1','5','0','=','F','\x01',
            '1','1','=','O','1','\x01',
            '3','2','=','1','0','0','\x01',
            '3','1','=','1','.','0','0','0','\x01',
            '1','0','=','0','9','5','\x01',
        };

        std::memcpy(pkt.data.data(), data, sizeof(data));
        pkt.len        = 50;
        pkt.venue      = Venue::BIST;
        pkt.protocol   = Protocol::FIX;

        return pkt;
    }();
    OutPacket fix_outpacket_partial_4 = []() {
        OutPacket pkt{};

        const uint8_t data[] = {
            // ── New Order (D) ── offset: 0
            '8','=','F','I','X','.','4','.','2','\x01',
            '9','=','0','6','4','\x01',
            '3','5','=','D','\x01',
            '3','4','=','5','\x01',
            '4','9','=','C','L','I','E','N','T','\x01',
            '5','6','=','E','X','C','H','\x01',
            '1','1','=','O','1','\x01',
            '5','5','=','G','A','R','A','N','\x01',
        };

        std::memcpy(pkt.data.data(), data, sizeof(data));
        pkt.len        = 59;
        pkt.venue      = Venue::BIST;
        pkt.protocol   = Protocol::FIX;

        return pkt;
    }();
    OutPacket fix_outpacket_partial_5 = []() {
        OutPacket pkt{};

        const uint8_t data[] = {
            '3','8','=','1','0','0','\x01',
            '4','4','=','1','.','0','0','0','\x01',
        };

        std::memcpy(pkt.data.data(), data, sizeof(data));
        pkt.len        = 16;
        pkt.venue      = Venue::BIST;
        pkt.protocol   = Protocol::FIX;

        return pkt;
    }();
    OutPacket fix_outpacket_partial_6 = []() {
        OutPacket pkt{};

        const uint8_t data[] = {
            '5','4','=','1','\x01',
            '1','0','=','1','6','4','\x01',
            // ── Cancel (F) ── offset: 87
            '8','=','F','I','X','.','4','.','2','\x01',
            '9','=','0','3','1','\x01',
            '3','5','=','F','\x01',
            '3','4','=','6','\x01',
            '1','1','=','O','2','\x01',
            '4','1','=','O','1','\x01',
            '5','5','=','T','U','P','R','S','\x01',
            '1','0','=','0','7','1','\x01',
            // ── Execution Report (8) ── offset: 141
            '8','=','F','I','X','.','4','.','2','\x01',
            '9','=','0','4','3','\x01',
            '3','5','=','8','\x01',
            '3','4','=','4','\x01',
            '3','9','=','2','\x01',
            '1','5','0','=','F','\x01',
            '1','1','=','O','1','\x01',
            '3','2','=','1','0','0','\x01',
            '3','1','=','1','.','0','0','0','\x01',
            '1','0','=','0','9','5','\x01',
        };

        std::memcpy(pkt.data.data(), data, sizeof(data));
        pkt.len        = 132;
        pkt.venue      = Venue::BIST;
        pkt.protocol   = Protocol::FIX;

        return pkt;
    }();

    //============= OUCH BIST DATA ==================

    inline const std::vector<uint8_t> ouch_bist_pkt1 = {
        'A',                                                 // offset: 0
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,            // timestamp | offset: 1
        'T','O','K','E','N','0','0','0','0','0','0','0','0','2',  // order_token | offset: 9
        0x00,0x00,0x00,0x02,                                 // order_book_id | offset: 23
        'B',                                                 // side | offset: 27
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,            // order_id | offset: 28
        0x00,0x00,0x00,0x00,0x00,0x00,0x07,0xD0,            // quantity | offset: 36
        0x00,0x00,0x27,0x10,                                 // price | offset: 44
        0x00,                                                // time_in_force | offset: 48
        0x00,                                                // open_close | offset: 49
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,                    // client_account | offset: 50, len: 16
        0x01,                                                // order_state | offset: 66
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,                      // customer_info | offset: 67, len: 15
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,                    // exchange_info | offset: 82, len: 32
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,            // pre_trade_qty | offset: 114
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,            // display_qty | offset: 122
        0x00,                                                // client_category | offset: 130
        0x00,                                                // offhours | offset: 131
        0x00,                                                // smp_level | offset: 132
        0x00,                                                // smp_method | offset: 133
        0,0,0,                                               // smp_ID | offset: 134
    };
    inline const std::vector<uint8_t> ouch_bist_pkt2 = {
        'C',                                                 // offset: 0
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,            // timestamp | offset: 1
        'T','O','K','E','N','0','0','0','0','0','0','0','0','2',  // order_token | offset: 9
        0x00,0x00,0x00,0x02,                                 // order_book_id | offset: 23
        'B',                                                 // side | offset: 27
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,            // order_id | offset: 28
        0x00,                                                // reason | offset: 36
    };
    inline const std::vector<uint8_t> ouch_bist_pkt3 = {
        'E',                                                 // offset: 0
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,            // timestamp | offset: 1
        'T','O','K','E','N','0','0','0','0','0','0','0','0','2',  // order_token | offset: 9
        0x00,0x00,0x00,0x02,                                 // order_book_id | offset: 23
        0x00,0x00,0x00,0x00,0x00,0x00,0x03,0xE8,            // traded_quantity | offset: 27
        0x00,0x00,0x27,0x10,                                 // trade_price | offset: 35
        0,0,0,0,0,0,0,0,0,0,0,0,                             // match_id | offset: 39, len: 12
        0x00,                                                // client_category | offset: 51
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,                    // reserved | offset: 52, len: 16
    };

    OutPacket ouch_bist_outpacket_partial_1 = []() {
        OutPacket pkt{};

        const uint8_t data[] = {
        // OrderAccepted Message
        0x00,0x8A,                                           // Pkt length after this field
        'S',                                                 // SBT SequencedPkt (S)
        'A',                                                 // offset: 0
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,            // timestamp | offset: 1
        'T','O','K','E','N','0','0','0','0','0','0','0','0','2',  // order_token | offset: 9
        0x00,0x00,0x00,0x02,                                 // order_book_id | offset: 23
        'B',                                                 // side | offset: 27
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,            // order_id | offset: 28

        };

        std::memcpy(pkt.data.data(), data, sizeof(data));
        pkt.len        = 39;
        pkt.venue      = Venue::BIST;
        pkt.protocol   = Protocol::OUCH;

        return pkt;
    }();
    OutPacket ouch_bist_outpacket_partial_2 = []() {
        OutPacket pkt{};

        const uint8_t data[] = {
        0x00,0x00,0x00,0x00,0x00,0x00,0x07,0xD0,            // quantity | offset: 36
        0x00,0x00,0x27,0x10,                                 // price | offset: 44
        0x00,                                                // time_in_force | offset: 48
        0x00,                                                // open_close | offset: 49
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,                    // client_account | offset: 50, len: 16
        0x01,                                                // order_state | offset: 66
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,                      // customer_info | offset: 67, len: 15
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,                    // exchange_info | offset: 82, len: 32
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,            // pre_trade_qty | offset: 114
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,            // display_qty | offset: 122
        0x00,                                                // client_category | offset: 130
        0x00,                                                // offhours | offset: 131
        0x00,                                                // smp_level | offset: 132
        0x00,                                                // smp_method | offset: 133
        0,0,0,                                               // smp_ID | offset: 134

        // Cancelled Message
        0x00,0x26,                                            // Pkt length after this field
        'S',                                                  // SBT SequencedPkt (S)
        'C',                                                 // offset: 0
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,            // timestamp | offset: 1
        'T','O','K','E','N','0','0','0','0','0','0','0','0','2',  // order_token | offset: 9
        0x00,0x00,0x00,0x02,                                 // order_book_id | offset: 23
        'B',                                                 // side | offset: 27
        0x00,0x00,                                          // order_id | offset: 28
        };

        std::memcpy(pkt.data.data(), data, sizeof(data));
        pkt.len        = 134;
        pkt.venue      = Venue::BIST;
        pkt.protocol   = Protocol::OUCH;

        return pkt;
    }();
    OutPacket ouch_bist_outpacket_partial_3 = []() {
        OutPacket pkt{};

        const uint8_t data[] = {
        0x00,0x00,0x00,0x00,0x00,0x01,                      
        0x00,                                                // reason | offset: 36

        // Executed Message
        0x00,0x45,                                          // Pkt length after this field
        'S',                                                // SBT SequencedPkt (S)
        'E',                                                 // offset: 0
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,            // timestamp | offset: 1
        'T','O','K','E','N','0','0','0','0','0','0','0','0','2',  // order_token | offset: 9
        0x00,0x00,0x00,0x02,                                 // order_book_id | offset: 23
        0x00,0x00,0x00,0x00,0x00,0x00,0x03,0xE8,            // traded_quantity | offset: 27
        0x00,0x00,0x27,0x10,                                 // trade_price | offset: 35
        0,0,0,0,0,0,0,0,0,0,0,0,                             // match_id | offset: 39, len: 12
        0x00,                                                // client_category | offset: 51
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,                    // reserved | offset: 52, len: 16
        };

        std::memcpy(pkt.data.data(), data, sizeof(data));
        pkt.len        = 78;
        pkt.venue      = Venue::BIST;
        pkt.protocol   = Protocol::OUCH;

        return pkt;
    }();
    OutPacket ouch_bist_outpacket_partial_4 = []() {
        OutPacket pkt{};

        const uint8_t data[] = {
        // Executed Message 
        0x00,                                               // Pkt length after this field (1st byte)
        };
        std::memcpy(pkt.data.data(), data, sizeof(data));
        pkt.len        = 1;
        pkt.venue      = Venue::BIST;
        pkt.protocol   = Protocol::OUCH;

        return pkt;
    }();
    OutPacket ouch_bist_outpacket_partial_5 = []() {
        OutPacket pkt{};

        const uint8_t data[] = {                                               
        // Executed Message
        0x45,                                                // Pkt length after this field (2nd byte)
        'S',                                                 // SBT SequencedPkt (S)
        'E',                                                 // offset: 0
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,            // timestamp | offset: 1
        'T','O','K','E','N','0','0','0','0','0','0','0','0','2',  // order_token | offset: 9
        0x00,0x00,0x00,0x02,                                 // order_book_id | offset: 23
        0x00,0x00,0x00,0x00,0x00,0x00,0x03,0xE8,            // traded_quantity | offset: 27
        0x00,0x00,0x27,0x10,                                 // trade_price | offset: 35
        0,0,0,0,0,0,0,0,0,0,0,0,                             // match_id | offset: 39, len: 12
        0x00,                                                // client_category | offset: 51
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,                    // reserved | offset: 52, len: 16
        };

        std::memcpy(pkt.data.data(), data, sizeof(data));
        pkt.len        = 70;
        pkt.venue      = Venue::BIST;
        pkt.protocol   = Protocol::OUCH;

        return pkt;
    }();


    //============= OUCH NASDAQ DATA ==================

    inline const std::vector<uint8_t> ouch_nasdaq_pkt1 = {
        0x00,0x00,0x00,  //sbt header(not parsed for this pkt)
        'A',                                                // message_type | offset: 0
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,           // timestamp | offset: 1
        0x00,0x00,0x00,0x01,                                // user_ref_num | offset: 9
        'B',                                                // side | offset: 13
        0x00,0x00,0x03,0xE8,                                // quantity | offset: 14
        'A','A','P','L',' ',' ',' ',' ',                    // symbol | offset: 18
        0x00,0x00,0x00,0x00,0x00,0x00,0x27,0x10,           // price | offset: 26
        '0',                                                // time_in_force | offset: 34
        'Y',                                                // display | offset: 35
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,           // order_reference_number | offset: 36
        'A',                                                // capacity | offset: 44
        'N',                                                // intermarket_sweep_eligibility | offset: 45
        'N',                                                // cross_type | offset: 46
        'L',                                                // order_state | offset: 47
        'C','L','I','E','N','T','0','0','0','0','0','0','1',' ', // cl_ord_id | offset: 48
        0x00,0x00,                                          // appendage_length | offset: 62
    };
    inline const std::vector<uint8_t> ouch_nasdaq_pkt2 = {
        0x00,0x00,0x00,  //sbt header(not parsed for this pkt)
        'C',                                                // message_type | offset: 0
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,           // timestamp | offset: 1
        0x00,0x00,0x00,0x02,                                // user_ref_num | offset: 9
        0x00,0x00,0x01,0xF4,                                // quantity | offset: 13
        'U',                                                // reason | offset: 17
        0x00,0x00,                                          // appendage_length | offset: 18
    };
    inline const std::vector<uint8_t> ouch_nasdaq_pkt3 = {
        0x00,0x00,0x00,  //sbt header(not parsed for this pkt)
        'U',                                                // message_type | offset: 0
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,           // timestamp | offset: 1
        0x00,0x00,0x00,0x01,                                // orig_user_ref_num | offset: 9
        0x00,0x00,0x00,0x02,                                // user_ref_num | offset: 13
        'B',                                                // side | offset: 17
        0x00,0x00,0x03,0xE8,                                // quantity | offset: 18
        'A','A','P','L',' ',' ',' ',' ',                    // symbol | offset: 22
        0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x39,           // price | offset: 30
        '0',                                                // time_in_force | offset: 38
        'Y',                                                // display | offset: 39
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,           // order_reference_number | offset: 40
        'A',                                                // capacity | offset: 48
        'N',                                                // intermarket_sweep_eligibility | offset: 49
        'N',                                                // cross_type | offset: 50
        'L',                                                // order_state | offset: 51
        'C','L','I','E','N','T','0','0','0','0','0','0','2',' ', // cl_ord_id | offset: 52
        0x00,0x00,                                          // appendage_length | offset: 66
    };

    //============= ITCH BIST DATA ==================

    inline const std::vector<uint8_t> itch_bist_pkt1 = {
        0x52,                                           // message_type = 'R' | offset: 0
        0x00,0x00,0x00,0x30,                            // timestamp_ns = 48 | offset: 1, len: 4
        0x00,0x00,0x00,0x03,                            // order_book_id = 3 | offset: 5, len: 4
        'G','A','R','A','N',' ',' ',' ',                // symbol (32 bytes) | offset: 9, len: 32
        ' ',' ',' ',' ',' ',' ',' ',' ',
        ' ',' ',' ',' ',' ',' ',' ',' ',
        ' ',' ',' ',' ',' ',' ',' ',' ',
        'G','a','r','a','n','t','i',' ',                // long_name (32 bytes) | offset: 41, len: 32
        ' ',' ',' ',' ',' ',' ',' ',' ',
        ' ',' ',' ',' ',' ',' ',' ',' ',
        ' ',' ',' ',' ',' ',' ',' ',' ',
        'T','R','G','A','R','A','N','T','I','S','0','1', // isin (12 bytes) | offset: 73, len: 12
        0x05,                                           // financial_product = 5 (Cash) | offset: 85, len: 1
        'T','R','Y',                                    // trading_currency (3 bytes) | offset: 86, len: 3
        0x00,0x02,                                      // decimals_in_price = 2 | offset: 89, len: 2
        0x00,0x02,                                      // decimals_in_nominal = 2 | offset: 91, len: 2
        0x00,0x00,0x00,0x01,                            // odd_lot_size = 1 | offset: 93, len: 4
        0x00,0x00,0x00,0x01,                            // round_lot_size = 1 | offset: 97, len: 4
        0x00,0x00,0x00,0x01,                            // block_lot_size = 1 | offset: 101, len: 4
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,        // nominal_value = 0 | offset: 105, len: 8
        0x00,                                           // number_of_legs = 0 | offset: 113, len: 1
        0x00,0x00,0x00,0x00,                            // underlying_order_book_id = 0 | offset: 114, len: 4
        0x00,0x00,0x00,0x00,                            // strike_price = 0 | offset: 118, len: 4
        0x00,0x00,0x00,0x00,                            // expiration_date = 0 | offset: 122, len: 4
        0x00,0x02,                                      // decimals_in_strike_price = 2 | offset: 126, len: 2
        0x00,                                           // put_or_call = 0 | offset: 128, len: 1
        0x01,                                           // ranking_type = 1 | offset: 129, len: 1
    };
    inline const std::vector<uint8_t> itch_bist_pkt2 = {
        0x41,                         // 'A'
        0x00,0x00,0x00,0x10,          // timestamp_ns = 16
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01, // order_id = 1
        0x00,0x00,0x00,0x02,          // order_book_id = 2
        0x42,                         // side = 'B'
        0x00,0x00,0x00,0x01,          // ranking_seq_number = 1
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x64, // quantity = 100
        0x00,0x00,0x00,0x32,          // price = 50
        0x00,0x01,                    // order_attributes
        0x02,                         // lot_type
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10 // ranking_time
    };
    inline const std::vector<uint8_t> itch_bist_pkt3 = {
        0x45,                         // 'E'
        0x00,0x00,0x00,0x20,          // timestamp_ns
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01, // order_id
        0x00,0x00,0x00,0x02,          // order_book_id
        0x53,                         // side = 'S'
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x32, // executed_quantity = 50
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x99, // match_id
        0x00,0x00,0x00,0x03,          // combo_group_id
        0,0,0,0,0,0,0,                // reserved1
        0,0,0,0,0,0,0                 // reserved2
    };

    OutPacket itch_bist_outpacket_full_1 = []() {
        OutPacket pkt{};

        const uint8_t data[] = {
        // Executed Message 
        0x52,                                           // message_type = 'R' | offset: 0
        0x00,0x00,0x00,0x30,                            // timestamp_ns = 48 | offset: 1, len: 4
        0x00,0x00,0x00,0x03,                            // order_book_id = 3 | offset: 5, len: 4
        'G','A','R','A','N',' ',' ',' ',                // symbol (32 bytes) | offset: 9, len: 32
        ' ',' ',' ',' ',' ',' ',' ',' ',
        ' ',' ',' ',' ',' ',' ',' ',' ',
        ' ',' ',' ',' ',' ',' ',' ',' ',
        'G','a','r','a','n','t','i',' ',                // long_name (32 bytes) | offset: 41, len: 32
        ' ',' ',' ',' ',' ',' ',' ',' ',
        ' ',' ',' ',' ',' ',' ',' ',' ',
        ' ',' ',' ',' ',' ',' ',' ',' ',
        'T','R','G','A','R','A','N','T','I','S','0','1', // isin (12 bytes) | offset: 73, len: 12
        0x05,                                           // financial_product = 5 (Cash) | offset: 85, len: 1
        'T','R','Y',                                    // trading_currency (3 bytes) | offset: 86, len: 3
        0x00,0x02,                                      // decimals_in_price = 2 | offset: 89, len: 2
        0x00,0x02,                                      // decimals_in_nominal = 2 | offset: 91, len: 2
        0x00,0x00,0x00,0x01,                            // odd_lot_size = 1 | offset: 93, len: 4
        0x00,0x00,0x00,0x01,                            // round_lot_size = 1 | offset: 97, len: 4
        0x00,0x00,0x00,0x01,                            // block_lot_size = 1 | offset: 101, len: 4
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,        // nominal_value = 0 | offset: 105, len: 8
        0x00,                                           // number_of_legs = 0 | offset: 113, len: 1
        0x00,0x00,0x00,0x00,                            // underlying_order_book_id = 0 | offset: 114, len: 4
        0x00,0x00,0x00,0x00,                            // strike_price = 0 | offset: 118, len: 4
        0x00,0x00,0x00,0x00,                            // expiration_date = 0 | offset: 122, len: 4
        0x00,0x02,                                      // decimals_in_strike_price = 2 | offset: 126, len: 2
        0x00,                                           // put_or_call = 0 | offset: 128, len: 1
        0x01,                                           // ranking_type = 1 | offset: 129, len: 1

        };
        std::memcpy(pkt.data.data(), data, sizeof(data));
        pkt.len        = 130;
        pkt.venue      = Venue::BIST;
        pkt.protocol   = Protocol::ITCH;

        return pkt;
    }();

    //============= ITCH NASDAQ DATA ==================

    inline const std::vector<uint8_t> itch_nasdaq_pkt1 = {
        0x52,                               // message_type = 'R' | offset: 0
        0x00,0x01,                          // stock_locate = 1 | offset: 1, len: 2
        0x00,0x02,                          // tracking_number = 2 | offset: 3, len: 2
        0x00,0x00,0x00,0x00,0x00,0x30,      // timestamp = 48 | offset: 5, len: 6
        'A','A','P','L',' ',' ',' ',' ',    // stock = "AAPL" | offset: 11, len: 8
        'Q',                                // market_category | offset: 19, len: 1
        'N',                                // financial_status_indicator | offset: 20, len: 1
        0x00,0x00,0x00,0x64,                // round_lot_size = 100 | offset: 21, len: 4
        'N',                                // round_lots_only | offset: 25, len: 1
        'C',                                // issue_classification | offset: 26, len: 1
        'Z','Z',                            // issue_sub_type | offset: 27, len: 2
        'P',                                // authenticity | offset: 29, len: 1
        'N',                                // short_sale_threshold_indicator | offset: 30, len: 1
        'N',                                // ipo_flag | offset: 31, len: 1
        '1',                                // luld_reference_price_tier | offset: 32, len: 1
        'N',                                // etp_flag | offset: 33, len: 1
        0x00,0x00,0x00,0x01,                // etp_leverage_factor = 1 | offset: 34, len: 4
        'N',                                // inverse_indicator | offset: 38, len: 1
    };
    inline const std::vector<uint8_t> itch_nasdaq_pkt2 = {
        0x41,                   // 'A'
        0x00,0x01,              // stock_locate = 1
        0x00,0x02,              // tracking_number = 2                                       
        0x00,0x00,0x00,0x00,0x00,0x64,  // timestamp (6 byte): 100
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01, // order_ref (8 byte)
        0x42,                   // side = 'B'
        0x00,0x00,0x00,0x64,    // shares 100
        0x41,0x41,0x50,0x4C,0x20,0x20,0x20,0x20, // stock "AAPL    "
        0x00,0x00,0x27,0x10     // price 10000 (1.0000$)
    };
    inline const std::vector<uint8_t> itch_nasdaq_pkt3 = {
        0x45,                   // 'E'
        0x00,0x01,              // stock_locate
        0x00,0x03,              // tracking_number
        0x00,0x00,0x00,0x00,0x00,0xC8, // timestamp 200
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01, // order_ref
        0x00,0x00,0x00,0x32,    // executed shares 50
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x99   // match_number
    };

    OutPacket itch_nasdaq_outpacket_full_1 = []() {
        OutPacket pkt{};

        const uint8_t data[] = {                                               
        0x52,                               // message_type = 'R' | offset: 0
        0x00,0x01,                          // stock_locate = 1 | offset: 1, len: 2
        0x00,0x02,                          // tracking_number = 2 | offset: 3, len: 2
        0x00,0x00,0x00,0x00,0x00,0x30,      // timestamp = 48 | offset: 5, len: 6
        'A','A','P','L',' ',' ',' ',' ',    // stock = "AAPL" | offset: 11, len: 8
        'Q',                                // market_category | offset: 19, len: 1
        'N',                                // financial_status_indicator | offset: 20, len: 1
        0x00,0x00,0x00,0x64,                // round_lot_size = 100 | offset: 21, len: 4
        'N',                                // round_lots_only | offset: 25, len: 1
        'C',                                // issue_classification | offset: 26, len: 1
        'Z','Z',                            // issue_sub_type | offset: 27, len: 2
        'P',                                // authenticity | offset: 29, len: 1
        'N',                                // short_sale_threshold_indicator | offset: 30, len: 1
        'N',                                // ipo_flag | offset: 31, len: 1
        '1',                                // luld_reference_price_tier | offset: 32, len: 1
        'N',                                // etp_flag | offset: 33, len: 1
        0x00,0x00,0x00,0x01,                // etp_leverage_factor = 1 | offset: 34, len: 4
        'N',                                // inverse_indicator | offset: 38, len: 1
        };

        std::memcpy(pkt.data.data(), data, sizeof(data));
        pkt.len        = 39;
        pkt.venue      = Venue::NASDAQ;
        pkt.protocol   = Protocol::ITCH;

        return pkt;
    }();

//===========================================================================
//                  ORDER MANAGER TEST DATA
//===========================================================================

    // FIX - GARAN | cl_ord_id="F1" | price=15000 | qty=500
    inline FIXMessage make_fix_new_order() {
        FIXMessage m{};
        m.msg_type       = '8';
        m.seqnum         = 1;
        m.quantity       = 500;
        m.ord_status     = '0';   
        m.exec_type      = '0'; 
        m.price          = 15000;   
        m.side           = 1;       
        m.ord_type       = '2';       
        m.time_in_force  = 0;       
        std::strncpy(m.symbol,    "GARAN", FIXMessage::SYMBOL_SIZE);
        std::strncpy(m.cl_ord_id, "F1",   FIXMessage::ID_SIZE);
        std::strncpy(m.order_id,  "O1",   FIXMessage::ID_SIZE);
        std::strncpy(m.exec_id,   "E1",    FIXMessage::ID_SIZE);
        return m;
    }
    inline FIXMessage make_fix_partial_fill() {
        FIXMessage m{};
        m.msg_type       = '8';
        m.seqnum         = 2;
        m.exec_type      = 'F';  
        m.side           = 1;       
        m.ord_status     = '1';    
        m.last_qty       = 200;     
        m.last_price     = 15000;
        m.filled_qty     = 200;     
        m.leaves_qty     = 300;    
        m.cxl_rej_reason = 255;
        std::strncpy(m.cl_ord_id, "F1", FIXMessage::ID_SIZE);
        std::strncpy(m.symbol,  "GARAN", FIXMessage::SYMBOL_SIZE);
        std::strncpy(m.order_id,  "O1",    FIXMessage::ID_SIZE);
        std::strncpy(m.exec_id,   "E2",    FIXMessage::ID_SIZE);
        return m;
    }
    inline FIXMessage make_fix_cancelled_confirm() {
        FIXMessage m{};
        m.msg_type       = '8';
        m.seqnum         = 3;       
        m.exec_type      = '4';     
        m.ord_status     = '4';
        m.side           = 1;        
        m.last_qty       = 0;
        m.last_price     = 0;
        m.filled_qty     = 200;
        m.leaves_qty     = 0;
        m.cxl_rej_reason = 255;
        std::strncpy(m.cl_ord_id, "F1",    FIXMessage::ID_SIZE);  
        std::strncpy(m.symbol,    "GARAN", FIXMessage::SYMBOL_SIZE);
        std::strncpy(m.order_id,  "O1",    FIXMessage::ID_SIZE);
        std::strncpy(m.exec_id,   "E3",    FIXMessage::ID_SIZE);
        return m;
    }

    // BIST ITCH - Stock: GARAN | order_book_id: 3 | price: 10000 | quantity: 1000
    inline BIST::ITCHOrderBookDirectoryMessage make_itch_bist_directory() {
        BIST::ITCHOrderBookDirectoryMessage m{};
        m.message_type              = 'R';
        m.timestamp_ns              = 48;
        m.order_book_id             = 3;
        m.financial_product         = 5;       
        m.decimals_in_price         = 2;
        m.decimals_in_nominal       = 2;
        m.decimals_in_strike_price  = 2;
        m.odd_lot_size              = 1;
        m.round_lot_size            = 10;
        m.block_lot_size            = 1;
        m.nominal_value             = 0;
        m.number_of_legs            = 0;
        m.underlying_order_book_id  = 0;
        m.strike_price              = 0;
        m.expiration_date           = 0;
        m.put_or_call               = 0;
        m.ranking_type              = 1;       
        std::strncpy(m.symbol,           "GARAN",          sizeof(m.symbol));
        std::strncpy(m.long_name,        "Garanti Bankasi", sizeof(m.long_name));
        std::strncpy(m.isin,             "TRGARAN000001",   sizeof(m.isin));
        std::strncpy(m.trading_currency, "TRY",             sizeof(m.trading_currency));
        return m;
    }
    inline BIST::ITCHAddOrderMessage make_itch_bist_add_order() {
        BIST::ITCHAddOrderMessage m{};
        m.message_type       = 'A';
        m.timestamp_ns       = 100;
        m.order_book_id      = 3;       
        m.order_id           = 42;
        m.side               = 'B';    
        m.quantity           = 1000;
        m.price              = 10000;   
        return m;
    }
    inline BIST::ITCHOrderExecutedMessage make_itch_bist_executed() {
        BIST::ITCHOrderExecutedMessage m{};
        m.message_type    = 'E';
        m.timestamp_ns    = 200;
        m.order_book_id   = 3;          
        m.order_id        = 42;         
        m.side            = 'B';
        m.executed_quantity = 600;
        return m;
    }
    inline BIST::ITCHOrderDeleteMessage make_itch_bist_cancelled() {
        BIST::ITCHOrderDeleteMessage m{};
        m.message_type  = 'D';
        m.timestamp_ns  = 300;
        m.order_book_id = 3;           
        m.order_id      = 42;           
        m.side          = 'B';
        return m;
    }
    inline BIST::ITCHAddOrderMessage make_itch_bist_add_order_2() {
        BIST::ITCHAddOrderMessage m{};
        m.message_type       = 'A';
        m.timestamp_ns       = 100;
        m.order_book_id      = 3;       
        m.order_id           = 43;
        m.side               = 'B';    
        m.quantity           = 100;
        m.price              = 100;   
        return m;
    }
    inline BIST::ITCHAddOrderMessage make_itch_bist_add_order_3() {
        BIST::ITCHAddOrderMessage m{};
        m.message_type       = 'A';
        m.timestamp_ns       = 100;
        m.order_book_id      = 3;       
        m.order_id           = 44;
        m.side               = 'B';    
        m.quantity           = 1000;
        m.price              = 1000;   
        return m;
    }
    inline BIST::ITCHAddOrderMessage make_itch_bist_add_order_4() {
        BIST::ITCHAddOrderMessage m{};
        m.message_type       = 'A';
        m.timestamp_ns       = 100;
        m.order_book_id      = 3;       
        m.order_id           = 45;
        m.side               = 'S';    
        m.quantity           = 1000;
        m.price              = 1000;   
        return m;
    }

    // BIST OUCH - GARAN | order_book_id=3 | order_id=42 | token: "TOKEN000000042" | price: 10000 | quantity: 100

    inline BIST::OUT::OUCHOrderAcceptedMessage make_ouch_bist_accepted() {
        BIST::OUT::OUCHOrderAcceptedMessage m{};
        m.message_type  = 'A';
        m.timestamp     = 100;          
        m.order_book_id = 3;            
        m.order_id      = 42;
        m.quantity      = 100;
        m.price         = 10000;        
        m.side          = 'B';
        m.order_state   = 1;            
        m.time_in_force = 0;            
        m.open_close    = 0;
        m.client_category = 0;
        m.offhours      = 0;
        m.smp_level     = 0;
        m.smp_method    = 0;
        m.pre_trade_qty = 0;
        m.display_qty   = 0;
        std::strncpy(m.order_token, "TOKEN000000042", sizeof(m.order_token));
        return m;
    }
    inline BIST::OUT::OUCHOrderExecutedMessage make_ouch_bist_executed() {
        BIST::OUT::OUCHOrderExecutedMessage m{};
        m.message_type    = 'E';
        m.timestamp       = 200;        
        m.order_book_id   = 3;         
        m.traded_quantity = 60;       
        m.trade_price     = 10000;     
        m.client_category = 0;
        std::strncpy(m.order_token, "TOKEN000000042", sizeof(m.order_token));
        return m;
    }
    inline BIST::OUT::OUCHOrderCancelledMessage make_ouch_bist_cancelled() {
        BIST::OUT::OUCHOrderCancelledMessage m{};
        m.message_type  = 'C';
        m.timestamp     = 300;          
        m.order_book_id = 3;           
        m.order_id      = 42;
        m.side          = 'B';
        m.reason        = 0;          
        std::strncpy(m.order_token, "TOKEN000000042", sizeof(m.order_token));
        return m;
    }

    // NASDAQ ITCH - AAPL | stock_locate=1 | order_ref=99 | price=10000 | shares=100
    inline NASDAQ::ITCHStockDirectoryMessage make_itch_nasdaq_directory() {
        NASDAQ::ITCHStockDirectoryMessage m{};
        m.message_type                  = 'R';
        m.stock_locate                  = 1;
        m.tracking_number               = 2;
        m.timestamp                     = 48;
        m.round_lot_size                = 10;
        m.market_category               = 'Q';   
        m.financial_status_indicator    = 'N';   
        m.round_lots_only               = 'N';
        m.issue_classification          = 'C';   
        m.issue_sub_type[0]             = 'Z';
        m.issue_sub_type[1]             = 'Z';
        m.authenticity                  = 'P';   
        m.short_sale_threshold_indicator = 'N';
        m.ipo_flag                      = 'N';
        m.luld_reference_price_tier     = '1';
        m.etp_flag                      = 'N';
        m.etp_leverage_factor           = 1;
        m.inverse_indicator             = 'N';
        std::strncpy(m.stock, "AAPL    ", sizeof(m.stock)); // space-padded 8 char
        return m;
    }
    inline NASDAQ::ITCHAddOrderMessage make_itch_nasdaq_add_order() {
        NASDAQ::ITCHAddOrderMessage m{};
        m.message_type    = 'A';
        m.stock_locate    = 1;      
        m.tracking_number = 2;
        m.timestamp       = 100;
        m.order_ref       = 99;
        m.side            = 'B';
        m.shares          = 100;
        m.price           = 10000;   // $1->0000 (1/10000 USD)
        std::strncpy(m.stock, "AAPL    ", sizeof(m.stock));
        return m;
    }
    inline NASDAQ::ITCHExecutedMessage make_itch_nasdaq_executed() {
        NASDAQ::ITCHExecutedMessage m{};
        m.message_type    = 'E';
        m.stock_locate    = 1;       
        m.tracking_number = 2;
        m.timestamp       = 200;
        m.order_ref       = 99;      
        m.executed_shares = 60;
        m.match_number    = 9901;
        return m;
    }
    inline NASDAQ::ITCHDeleteMessage make_itch_nasdaq_deleted() {
        NASDAQ::ITCHDeleteMessage m{};
        m.message_type    = 'D';
        m.stock_locate    = 1;       
        m.tracking_number = 2;
        m.timestamp       = 300;
        m.order_ref       = 99;      
        return m;
    }

    // NASDAQ OUCH - AAPL | stock_locate=1 | user_ref_num=99 | price=10000 | quantity=1000
    inline NASDAQ::OUT::OUCHOrderAcceptedMessage make_ouch_nasdaq_accepted() {
        NASDAQ::OUT::OUCHOrderAcceptedMessage m{};
        m.message_type              = 'A';
        m.timestamp                 = 100;      
        m.user_ref_num              = 99;      
        m.quantity                  = 1000;
        m.price                     = 10000;
        m.order_reference_number    = 99;      
        m.side                      = 'B';
        m.time_in_force             = '0';      
        std::strncpy(m.symbol,    "AAPL    ", sizeof(m.symbol));    // space-padded
        std::strncpy(m.cl_ord_id, "CLIENT0000001", sizeof(m.cl_ord_id));
        return m;
    }
    inline NASDAQ::OUT::OUCHOrderExecutedMessage make_ouch_nasdaq_executed() {
        NASDAQ::OUT::OUCHOrderExecutedMessage m{};
        m.message_type     = 'E';
        m.timestamp        = 200;      
        m.user_ref_num     = 99;        
        m.quantity         = 600;      
        m.price            = 10000;
        m.match_number     = 9901;     
        m.liquidity_flag   = 'A';       
        m.appendage_length = 0;
        return m;
    }
    inline NASDAQ::OUT::OUCHOrderCancelledMessage make_ouch_nasdaq_cancelled() {
        NASDAQ::OUT::OUCHOrderCancelledMessage m{};
        m.message_type     = 'C';
        m.timestamp        = 300;       
        m.user_ref_num     = 99;       
        m.quantity         = 400;      
        m.reason           = 'U';     
        return m;
    }

    // FIX
    inline FIXMessage fix_new = make_fix_new_order();
    inline FIXMessage fix_partial = make_fix_partial_fill();
    inline FIXMessage fix_cancel = make_fix_cancelled_confirm();

    // BIST ITCH 
    inline auto bist_dir      = make_itch_bist_directory();
    inline auto bist_add      = make_itch_bist_add_order();
    inline auto bist_exec     = make_itch_bist_executed();
    inline auto bist_cancel   = make_itch_bist_cancelled();
    inline auto bist_add_2    = make_itch_bist_add_order_2();
    inline auto bist_add_3    = make_itch_bist_add_order_3();
    inline auto bist_add_4    = make_itch_bist_add_order_4();

    // NASDAQ ITCH
    inline auto nasdaq_dir    = make_itch_nasdaq_directory();
    inline auto nasdaq_add    = make_itch_nasdaq_add_order();
    inline auto nasdaq_exec   = make_itch_nasdaq_executed();
    inline auto nasdaq_delete = make_itch_nasdaq_deleted();

    // BIST OUCH
    inline auto bist_acc   = make_ouch_bist_accepted();
    inline auto bist_execO = make_ouch_bist_executed();
    inline auto bist_canO  = make_ouch_bist_cancelled();

    // NASDAQ OUCH
    inline auto nasdaq_acc   = make_ouch_nasdaq_accepted();
    inline auto nasdaq_execO = make_ouch_nasdaq_executed();
    inline auto nasdaq_canO  = make_ouch_nasdaq_cancelled();


//===========================================================================
//                  STORE TO RISK RISK TEST DATA
//===========================================================================

    // ========================== FIX NEW ==========================
    inline auto make_fixNew()
    {
        auto* fixNew = new Order();

        fixNew->price = 15000;
        fixNew->quantity = 50;
        fixNew->remaining_quantity = 50;
        fixNew->symbol_index = 0;
        fixNew->side = Side::Buy;
        fixNew->status = Status::New;
        fixNew->venue = Venue::BIST;
        fixNew->isOurOrder = true;
        fixNew->protocol = Protocol::FIX;
        fixNew->order_type = OrderType::Limit;
        fixNew->syncState = SyncState::NewSeen;

        return fixNew;
    }

    // ========================== FIX PARTIAL ==========================
    inline auto make_fixPartial()
    {
        auto* fixPartial = new Order();

        fixPartial->price = 15000;
        fixPartial->quantity = 50;
        fixPartial->filled_quantity = 20;
        fixPartial->remaining_quantity = 30;
        fixPartial->last_exec_quantity = 20;
        fixPartial->symbol_index = 0;
        fixPartial->side = Side::Buy;
        fixPartial->status = Status::Partial;
        fixPartial->venue = Venue::BIST;
        fixPartial->isOurOrder = true;
        fixPartial->protocol = Protocol::FIX;
        fixPartial->order_type = OrderType::Limit;
        fixPartial->syncState = SyncState::NewSeen;

        return fixPartial;
    }

    // ========================== FIX CANCEL ==========================
    inline auto make_fixCancel()
    {
        auto* fixCancel = new Order();

        fixCancel->price = 15000;
        fixCancel->quantity = 50;
        fixCancel->filled_quantity = 20;
        fixCancel->remaining_quantity = 0;
        fixCancel->last_exec_quantity = 20;
        fixCancel->replaced_quantity = -30;
        fixCancel->symbol_index = 0;
        fixCancel->side = Side::Buy;
        fixCancel->status = Status::Deleted;
        fixCancel->venue = Venue::BIST;
        fixCancel->isOurOrder = true;
        fixCancel->protocol = Protocol::FIX;
        fixCancel->order_type = OrderType::Limit;
        fixCancel->syncState = SyncState::NewSeen;
        fixCancel->cancelled_count = 1;

        return fixCancel;
    }

    // ========================== BIST OUCH NEW ==========================
    inline auto make_ouchNew()
    {
        auto* ouchNew = new Order();

        ouchNew->price = 10000;
        ouchNew->quantity = 10;
        ouchNew->remaining_quantity = 10;
        ouchNew->symbol_index = 0;
        ouchNew->side = Side::Buy;
        ouchNew->status = Status::New;
        ouchNew->venue = Venue::BIST;
        ouchNew->isOurOrder = true;
        ouchNew->protocol = Protocol::OUCH;
        ouchNew->order_type = OrderType::Limit;

        return ouchNew;
    }

    // ========================== BIST OUCH EXEC ==========================
    inline auto make_ouchExec()
    {
        auto* ouchExec = new Order();

        ouchExec->price = 10000;
        ouchExec->quantity = 10;
        ouchExec->filled_quantity = 6;
        ouchExec->remaining_quantity = 4;
        ouchExec->last_exec_quantity = 6;
        ouchExec->symbol_index = 0;
        ouchExec->side = Side::Buy;
        ouchExec->status = Status::Partial;
        ouchExec->venue = Venue::BIST;
        ouchExec->isOurOrder = true;
        ouchExec->protocol = Protocol::OUCH;
        ouchExec->order_type = OrderType::Limit;

        return ouchExec;
    }

    // ========================== BIST OUCH CANCEL ==========================
    inline auto make_ouchCancel()
    {
        auto* ouchCancel = new Order();

        ouchCancel->price = 10000;
        ouchCancel->quantity = 10;
        ouchCancel->filled_quantity = 6;
        ouchCancel->remaining_quantity = 0;
        ouchCancel->last_exec_quantity = 6;
        ouchCancel->replaced_quantity = -4;
        ouchCancel->symbol_index = 0;
        ouchCancel->side = Side::Buy;
        ouchCancel->status = Status::Deleted;
        ouchCancel->venue = Venue::BIST;
        ouchCancel->isOurOrder = true;
        ouchCancel->protocol = Protocol::OUCH;
        ouchCancel->order_type = OrderType::Limit;
        ouchCancel->cancelled_count = 1;

        return ouchCancel;
    }

    // ========================== NASDAQ OUCH NEW ==========================
    inline auto make_nqOuchNew()
    {
        auto* nqOuchNew = new Order();

        nqOuchNew->price = 10000;
        nqOuchNew->quantity = 25;
        nqOuchNew->remaining_quantity = 25;
        nqOuchNew->symbol_index = 0;
        nqOuchNew->side = Side::Buy;
        nqOuchNew->status = Status::New;
        nqOuchNew->venue = Venue::NASDAQ;
        nqOuchNew->isOurOrder = true;
        nqOuchNew->protocol = Protocol::OUCH;
        nqOuchNew->order_type = OrderType::Limit;

        return nqOuchNew;
    }

    // ========================== NASDAQ OUCH EXEC ==========================
    inline auto make_nqOuchExec()
    {
        auto* nqOuchExec = new Order();

        nqOuchExec->price = 10000;
        nqOuchExec->quantity = 25;
        nqOuchExec->filled_quantity = 10;
        nqOuchExec->remaining_quantity = 15;
        nqOuchExec->last_exec_quantity = 10;
        nqOuchExec->symbol_index = 0;
        nqOuchExec->side = Side::Buy;
        nqOuchExec->status = Status::Partial;
        nqOuchExec->venue = Venue::NASDAQ;
        nqOuchExec->isOurOrder = true;
        nqOuchExec->protocol = Protocol::OUCH;
        nqOuchExec->order_type = OrderType::Limit;

        return nqOuchExec;
    }

    // ========================== NASDAQ OUCH CANCEL ==========================
    inline auto make_nqOuchCancel()
    {
        auto* nqOuchCancel = new Order();

        nqOuchCancel->price = 10000;
        nqOuchCancel->quantity = 25;
        nqOuchCancel->filled_quantity = 10;
        nqOuchCancel->last_exec_quantity = 10;
        nqOuchCancel->replaced_quantity = -15;
        nqOuchCancel->symbol_index = 0;
        nqOuchCancel->side = Side::Buy;
        nqOuchCancel->status = Status::Cancelled;
        nqOuchCancel->venue = Venue::NASDAQ;
        nqOuchCancel->isOurOrder = true;
        nqOuchCancel->protocol = Protocol::OUCH;
        nqOuchCancel->order_type = OrderType::Limit;
        nqOuchCancel->cancelled_count = 1;

        return nqOuchCancel;
    }

    // ========================== BIST ITCH ADD ==========================
    inline auto make_itchAdd()
    {
        auto* itchAdd = new Order();

        itchAdd->price = 10000;
        itchAdd->quantity = 1000;
        itchAdd->remaining_quantity = 1000;
        itchAdd->symbol_index = 0;
        itchAdd->side = Side::Buy;
        itchAdd->status = Status::New;
        itchAdd->venue = Venue::BIST;
        itchAdd->protocol = Protocol::ITCH;
        itchAdd->order_type = OrderType::Limit;

        return itchAdd;
    }

    // ========================== BIST ITCH EXEC ==========================
    inline auto make_itchExec()
    {
        auto* itchExec = new Order();

        itchExec->price = 10000;
        itchExec->quantity = 1000;
        itchExec->filled_quantity = 600;
        itchExec->remaining_quantity = 400;
        itchExec->last_exec_quantity = 600;
        itchExec->symbol_index = 0;
        itchExec->side = Side::Buy;
        itchExec->status = Status::Partial;
        itchExec->venue = Venue::BIST;
        itchExec->protocol = Protocol::ITCH;
        itchExec->order_type = OrderType::Limit;

        return itchExec;
    }

    // ========================== BIST ITCH DELETE ==========================
    inline auto make_itchDelete()
    {
        auto* itchDelete = new Order();

        itchDelete->price = 10000;
        itchDelete->quantity = 1000;
        itchDelete->filled_quantity = 600;
        itchDelete->remaining_quantity = 400;
        itchDelete->last_exec_quantity = 600;
        itchDelete->symbol_index = 0;
        itchDelete->side = Side::Buy;
        itchDelete->status = Status::Deleted;
        itchDelete->venue = Venue::BIST;
        itchDelete->protocol = Protocol::ITCH;
        itchDelete->order_type = OrderType::Limit;
        itchDelete->cancelled_count = 1;

        return itchDelete;
    }

    // ========================== NASDAQ ITCH ADD ==========================
    inline auto make_nqItchAdd()
    {
        auto* nqItchAdd = new Order();

        nqItchAdd->price = 10000;
        nqItchAdd->quantity = 100;
        nqItchAdd->remaining_quantity = 100;
        nqItchAdd->symbol_index = 0;
        nqItchAdd->side = Side::Buy;
        nqItchAdd->status = Status::New;
        nqItchAdd->venue = Venue::NASDAQ;
        nqItchAdd->protocol = Protocol::ITCH;
        nqItchAdd->order_type = OrderType::Limit;

        return nqItchAdd;
    }

    // ========================== NASDAQ ITCH EXEC ==========================
    inline auto make_nqItchExec()
    {
        auto* nqItchExec = new Order();

        nqItchExec->price = 10000;
        nqItchExec->quantity = 100;
        nqItchExec->filled_quantity = 60;
        nqItchExec->remaining_quantity = 40;
        nqItchExec->last_exec_quantity = 60;
        nqItchExec->symbol_index = 0;
        nqItchExec->side = Side::Buy;
        nqItchExec->status = Status::Partial;
        nqItchExec->venue = Venue::NASDAQ;
        nqItchExec->protocol = Protocol::ITCH;
        nqItchExec->order_type = OrderType::Limit;

        return nqItchExec;
    }

    // ========================== NASDAQ ITCH DELETE ==========================
    inline auto make_nqItchDelete()
    {
        auto* nqItchDelete = new Order();

        nqItchDelete->price = 10000;
        nqItchDelete->quantity = 100;
        nqItchDelete->filled_quantity = 60;
        nqItchDelete->remaining_quantity = 40;
        nqItchDelete->last_exec_quantity = 60;
        nqItchDelete->symbol_index = 0;
        nqItchDelete->side = Side::Buy;
        nqItchDelete->status = Status::Deleted;
        nqItchDelete->venue = Venue::NASDAQ;
        nqItchDelete->protocol = Protocol::ITCH;
        nqItchDelete->order_type = OrderType::Limit;
        nqItchDelete->cancelled_count = 1;

        return nqItchDelete;
    }

    inline auto* fixNew      = make_fixNew();
    inline auto* fixPartial  = make_fixPartial();
    inline auto* fixCancel   = make_fixCancel();

    inline auto* ouchNew     = make_ouchNew();
    inline auto* ouchExec    = make_ouchExec();
    inline auto* ouchCancel  = make_ouchCancel();

    inline auto* nqOuchNew   = make_nqOuchNew();
    inline auto* nqOuchExec  = make_nqOuchExec();
    inline auto* nqOuchCancel= make_nqOuchCancel();

    inline auto* itchAdd      = make_itchAdd();
    inline auto* itchExec     = make_itchExec();
    inline auto* itchDelete   = make_itchDelete();

    inline auto* nqItchAdd    = make_nqItchAdd();
    inline auto* nqItchExec   = make_nqItchExec();
    inline auto* nqItchDelete = make_nqItchDelete();

//===========================================================================
//                  STRATEGY TO RISK RISK TEST DATA
//===========================================================================
    inline auto make_order1() 
    {
        auto* o = new Order(); // PASS
        o->price           = 10000;
        o->quantity        = 100;
        o->side            = Side::Buy;
        o->venue           = Venue::BIST;
        o->instrument_id   = 3;
        o->symbol_index    = 0;
        o->order_type      = OrderType::Limit;
        return o;
    }
    inline auto make_order2() 
    {
        auto* o = new Order(); // FAIL - quantity < min_qty=10
        o->price           = 10000;
        o->side            = Side::Buy;
        o->venue           = Venue::BIST;
        o->instrument_id   = 3;
        o->symbol_index    = 0;
        o->order_type      = OrderType::Limit;
        o->quantity        = 5; // < min_qty=10
        return o;
    }
    inline auto make_order3() 
    {
        auto* o = new Order(); // FAIL - quantity > max_qty=200
        o->price           = 10000;
        o->side            = Side::Buy;
        o->venue           = Venue::BIST;
        o->instrument_id   = 3;
        o->symbol_index    = 0;
        o->order_type      = OrderType::Limit;
        o->quantity        = 210; // > max_qty=200
        return o;
    }
    inline auto make_order4() 
    {
        auto* o = new Order(); // FAIL - InvalidLotSize - quantity not multiple of round_lot=10
        o->price           = 10000;
        o->side            = Side::Buy;
        o->venue           = Venue::BIST;
        o->instrument_id   = 3;
        o->symbol_index    = 0;
        o->order_type      = OrderType::Limit;
        o->quantity        = 15; // round_lot=10 
        return o;
    }
    inline auto make_order5() 
    {
        auto* o = new Order(); // FAIL - NotionalValueExceedsThreshold - MaxPriceDeviationExceeds
        o->price           = 190000;
        o->side            = Side::Buy;
        o->venue           = Venue::BIST;
        o->instrument_id   = 3;
        o->symbol_index    = 0;
        o->order_type      = OrderType::Limit;
        o->quantity        = 180; 
        return o;
    }
    inline auto make_order6() 
    {
        auto* o = new Order(); // FAIL - TickSizeViolation - price not multiple of tick=10
        o->price           = 9003;
        o->quantity        = 10;
        o->side            = Side::Buy;
        o->venue           = Venue::BIST;
        o->instrument_id   = 3;
        o->symbol_index    = 0;
        o->order_type      = OrderType::Limit;
        return o;
    }
    inline auto make_order7() 
    {
        auto* o = new Order(); // FAIL - FatFinger > ratio_threshold
        o->quantity        = 110;
        o->side            = Side::Buy;
        o->venue           = Venue::BIST;
        o->instrument_id   = 3;
        o->symbol_index    = 0;
        o->order_type      = OrderType::Limit;
        o->price           = 21000; // ratio > 2
        return o;
    }
    inline auto make_order8() 
    {
        auto* o = new Order(); // FAIL - MaxPositionValue 
        o->price           = 10000;
        o->quantity        = 100;
        o->side            = Side::Buy;
        o->venue           = Venue::BIST;
        o->instrument_id   = 3;
        o->symbol_index    = 0;
        o->order_type      = OrderType::Limit;
        return o;
    }
    inline auto make_order9() 
    {
        auto* o = new Order(); // FAIL - MaxNotionalValue - price * qty > max_notional_value
        o->price           = 10000;
        o->side            = Side::Buy;
        o->venue           = Venue::BIST;
        o->instrument_id   = 3;
        o->symbol_index    = 0;
        o->order_type      = OrderType::Limit;
        o->quantity        = 200;
        return o;
    }
    inline auto make_order10() 
    {
        auto* o = new Order(); // FAIL - AccountBalanceInsufficient - price * qty > balance
        o->price           = 10000;
        o->quantity        = 100;
        o->side            = Side::Buy;
        o->venue           = Venue::BIST;
        o->instrument_id   = 3;
        o->symbol_index    = 0;
        o->order_type      = OrderType::Limit;
        return o;
    }
    inline auto make_order11() 
    {
        auto* o = new Order(); // FAIL - MaxOpenOrders - current_open_orders >= max_open_orders
        o->price           = 10000;
        o->quantity        = 100;
        o->side            = Side::Buy;
        o->venue           = Venue::BIST;
        o->instrument_id   = 3;
        o->symbol_index    = 0;
        o->order_type      = OrderType::Limit;
        return o;
    }
    inline auto make_order12() 
    {
        auto* o = new Order(); // FAIL - MaxOpenOrdersPerSymbol - current_open_orders_for_symbol >= max_open_orders_per_symbol
        o->price           = 11000;
        o->quantity        = 100;
        o->side            = Side::Buy;
        o->venue           = Venue::BIST;
        o->instrument_id   = 3;
        o->symbol_index    = 0;
        o->order_type      = OrderType::Limit;
        return o;
    }
    inline auto make_order13() 
    {
        auto* o = new Order(); // FAIL - SelfTrade - order would execute against our own order
        o->price          = 11000;
        o->quantity       = 100;
        o->side           = Side::Sell;
        o->venue          = Venue::BIST;
        o->instrument_id  = 3;
        o->symbol_index   = 0;
        o->order_type     = OrderType::Limit;
        return o;
    }
    inline auto make_order14() 
    {
        auto* o = new Order(); // FAIL - UnrealizedLossLimit
        o->price           = 20000;
        o->quantity        = 190;
        o->side            = Side::Buy;
        o->instrument_id   = 3;
        o->symbol_index    = 0;
        o->venue           = Venue::BIST;
        o->order_type      = OrderType::Limit;
        return o;
    }
    inline auto make_order15() 
    {
        auto* o = new Order(); // FAIL - RealizedLossLimit
        o->price         = 12000;
        o->quantity      = 100;
        o->side          = Side::Sell;
        o->instrument_id = 3;
        o->symbol_index  = 0;
        o->order_type    = OrderType::Limit;
        o->venue         = Venue::BIST;
        return o;
    }

    inline auto* o1 = make_order1(); 
    inline auto* o2 = make_order2();
    inline auto* o3 = make_order3();
    inline auto* o4 = make_order4();
    inline auto* o5 = make_order5();
    inline auto* o6 = make_order6();
    inline auto* o7 = make_order7();
    inline auto* o8 = make_order8();
    inline auto* o9 = make_order9();
    inline auto* o10 = make_order10();
    inline auto* o11 = make_order11();
    inline auto* o12 = make_order12();
    inline auto* o13 = make_order13();
    inline auto* o14 = make_order14();
    inline auto* o15 = make_order15();

//===========================================================================
//                  BUILDER TEST DATA
//===========================================================================
    inline auto make_fix_order() 
    {
        auto* o = new Order(); 
        o->price           = 123456;
        o->quantity        = 100;
        o->side            = Side::Buy;
        o->venue           = Venue::BIST;
        o->protocol        = Protocol::FIX;
        o->instrument_id   = 3;
        o->symbol_index    = 0;
        o->order_type      = OrderType::Limit;
        o->time_in_force   = TimeInForce::DAY;
        o->message_type    = static_cast<uint8_t>(FIXTypes::NewOrderSingle);
        o->isOurOrder      = true;
        o->timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        std::strncpy(o->symbol.data(), "GARAN", 5); 
        o->real_symbol_len = 5;    
        std::strncpy(o->client_order_token.data(), "CLIENT0000001", 13);
        o->real_cl_ord_token_len = 13;
        o->client_order_id = absl::Hash<std::string_view>{}(o->client_order_token.data());
        
        return o;
    }
    inline auto make_ouch_new_order() 
    {
        auto* o = new Order(); 
        o->price           = 10000;
        o->quantity        = 100;
        o->side            = Side::Buy;
        o->venue           = Venue::BIST;
        o->protocol        = Protocol::OUCH;
        o->instrument_id   = 3;
        o->symbol_index    = 0;
        o->order_type      = OrderType::Limit;
        o->time_in_force   = TimeInForce::DAY;
        o->message_type    = 0;
        o->isOurOrder      = true;
        o->timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        std::strncpy(o->symbol.data(), "GARAN", 5); 
        o->real_symbol_len = 5;   
        std::strncpy(o->client_order_token.data(), "CLIENT0000001", 13);
        o->real_cl_ord_token_len = 13;
        o->client_order_id = absl::Hash<std::string_view>{}(o->client_order_token.data());
        
        return o;
    }
    inline auto make_NQouch_new_order() 
    {
        auto* o = new Order(); 
        o->price           = 10000;
        o->quantity        = 100;
        o->side            = Side::Sell;
        o->venue           = Venue::NASDAQ;
        o->protocol        = Protocol::OUCH;
        o->instrument_id   = 1;
        o->symbol_index    = 0;
        o->order_type      = OrderType::Limit;
        o->time_in_force   = TimeInForce::DAY;
        o->message_type    = 0;
        o->isOurOrder      = true;
        o->timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        std::strncpy(o->symbol.data(), "AAPL", 4);
        o->real_symbol_len = 4;     
        std::strncpy(o->client_order_token.data(), "CLIENT0000001", 13);
        o->real_cl_ord_token_len = 13;
        o->client_order_id = absl::Hash<std::string_view>{}("CLIENT0000001");
        
        return o;
    }

    inline auto* fix_new_order = make_fix_order(); 
    inline auto* ouch_new_order = make_ouch_new_order();
    inline auto* NQouch_new_order = make_NQouch_new_order();

//===========================================================================
//                  NETWORK_IO TEST DATA
//===========================================================================
 
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

    inline const std::vector<uint8_t> ouch_bist_order_ack = {
        0x00, 0x8A,                                              // sbt packet_length = 138
        'S',                                                     // sbt packet_type = seqdata
        'A',                                                     // message_type | offset: 0
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,                 // timestamp | offset: 1
        'C','L','I','E','N','T','0','0','0','0','0','0','1',' ', // order_token | offset: 9, len: 14
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
        'C','L','I','E','N','T','0','0','0','0','0','0','1',' ', // cl_ord_id | offset: 48
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