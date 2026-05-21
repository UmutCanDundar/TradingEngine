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

namespace test_data_parser {

//===========================================================================
//                  PARSER TEST DATA
//===========================================================================

    //============= FIX DATA ==================

        inline const std::vector<uint8_t> single_fix_pkt1 = {
            // -------- New Order (D) PARTIAL --------
            '8','=','F','I','X','.','5','.','0','\x01',
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
            '8','=','F','I','X','.','5','.','0','\x01',
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

        RxPacket fix_RxPacket_single_1 = []() {
            RxPacket pkt{};

            const uint8_t data[] = {
                // ── New Order (D) ── offset: 0
                '8','=','F','I','X','.','5','.','0','\x01',
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
            };

            std::memcpy(pkt.data.data(), data, sizeof(data));
            pkt.len                = 87;
            pkt.venue              = Venue::BIST;
            pkt.protocol           = Protocol::FIX;
            pkt.release_this_pkt   = false;

            return pkt;
        }();

        RxPacket fix_RxPacket_full_1 = []() {
            RxPacket pkt{};

            const uint8_t data[] = {
                // ── New Order (D) ── offset: 0
                '8','=','F','I','X','.','5','.','0','\x01',
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
                '8','=','F','I','X','.','5','.','0','\x01',
                '9','=','0','3','1','\x01',
                '3','5','=','F','\x01',
                '3','4','=','2','\x01',
                '1','1','=','O','2','\x01',
                '4','1','=','O','1','\x01',
                '5','5','=','T','U','P','R','S','\x01',
                '1','0','=','0','7','1','\x01',
                // ── Execution Report (8) ── offset: 140
                '8','=','F','I','X','.','5','.','0','\x01',
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
            pkt.len                = 207;
            pkt.venue              = Venue::BIST;
            pkt.protocol           = Protocol::FIX;
            pkt.release_this_pkt   = false;

            return pkt;
        }();

        RxPacket fix_RxPacket_partial_1 = []() {
            RxPacket pkt{};

            const uint8_t data[] = {
                // ── New Order (D) ── offset: 0
                '8','=','F','I','X','.','5','.','0','\x01',
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
            pkt.len                = 84;
            pkt.venue              = Venue::BIST;
            pkt.protocol           = Protocol::FIX;
            pkt.release_this_pkt   = false;

            return pkt;
        }();
        RxPacket fix_RxPacket_partial_2 = []() {
            RxPacket pkt{};

            const uint8_t data[] = {
                '6','4','\x01',
                // ── Cancel (F) ── offset: 87
                '8','=','F','I','X','.','5','.','0','\x01',
                '9','=','0','3','1','\x01',
                '3','5','=','F','\x01',
                '3','4','=','2','\x01',
                '1','1','=','O','2','\x01',
                '4','1','=','O','1','\x01',
                '5','5','=','T','U','P','R','S','\x01',
                '1','0','=','0','7','1','\x01',
                // ── Execution Report (8) ── offset: 141
                '8','=','F','I','X','.','5','.','0','\x01',
                '9','=','0','4','3','\x01',
            };

            std::memcpy(pkt.data.data(), data, sizeof(data));
            pkt.len                = 73;
            pkt.venue              = Venue::BIST;
            pkt.protocol           = Protocol::FIX;
            pkt.release_this_pkt   = false;

            return pkt;
        }();
        RxPacket fix_RxPacket_partial_3 = []() {
            RxPacket pkt{};

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
            pkt.len                = 50;
            pkt.venue              = Venue::BIST;
            pkt.protocol           = Protocol::FIX;
            pkt.release_this_pkt   = false;

            return pkt;
        }();
        RxPacket fix_RxPacket_partial_4 = []() {
            RxPacket pkt{};

            const uint8_t data[] = {
                // ── New Order (D) ── offset: 0
                '8','=','F','I','X','.','5','.','0','\x01',
                '9','=','0','6','4','\x01',
                '3','5','=','D','\x01',
                '3','4','=','5','\x01',
                '4','9','=','C','L','I','E','N','T','\x01',
                '5','6','=','E','X','C','H','\x01',
                '1','1','=','O','1','\x01',
                '5','5','=','G','A','R','A','N','\x01',
            };

            std::memcpy(pkt.data.data(), data, sizeof(data));
            pkt.len                = 59;
            pkt.venue              = Venue::BIST;
            pkt.protocol           = Protocol::FIX;
            pkt.release_this_pkt   = false;

            return pkt;
        }();
        RxPacket fix_RxPacket_partial_5 = []() {
            RxPacket pkt{};

            const uint8_t data[] = {
                '3','8','=','1','0','0','\x01',
                '4','4','=','1','.','0','0','0','\x01',
            };

            std::memcpy(pkt.data.data(), data, sizeof(data));
            pkt.len                = 16;
            pkt.venue              = Venue::BIST;
            pkt.protocol           = Protocol::FIX;
            pkt.release_this_pkt   = false;

            return pkt;
        }();
        RxPacket fix_RxPacket_partial_6 = []() {
            RxPacket pkt{};

            const uint8_t data[] = {
                '5','4','=','1','\x01',
                '1','0','=','1','6','4','\x01',
                // ── Cancel (F) ── offset: 87
                '8','=','F','I','X','.','5','.','0','\x01',
                '9','=','0','3','1','\x01',
                '3','5','=','F','\x01',
                '3','4','=','6','\x01',
                '1','1','=','O','2','\x01',
                '4','1','=','O','1','\x01',
                '5','5','=','T','U','P','R','S','\x01',
                '1','0','=','0','7','1','\x01',
                // ── Execution Report (8) ── offset: 141
                '8','=','F','I','X','.','5','.','0','\x01',
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
            pkt.len                = 132;
            pkt.venue              = Venue::BIST;
            pkt.protocol           = Protocol::FIX;
            pkt.release_this_pkt   = false;

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

        RxPacket ouch_bist_RxPacket_single_1 = []() {
            RxPacket pkt{};

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

            std::memcpy(pkt.data.data(), data, sizeof(data));
            pkt.len                = 140;
            pkt.venue              = Venue::BIST;
            pkt.protocol           = Protocol::OUCH;
            pkt.release_this_pkt   = false;

            return pkt;
        }();

        RxPacket ouch_bist_RxPacket_full_1 = []() {
            RxPacket pkt{};

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
            pkt.len                = 251;
            pkt.venue              = Venue::BIST;
            pkt.protocol           = Protocol::OUCH;
            pkt.release_this_pkt   = false;

            return pkt;
        }();

        RxPacket ouch_bist_RxPacket_partial_1 = []() {
            RxPacket pkt{};

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
            pkt.len                = 39;
            pkt.venue              = Venue::BIST;
            pkt.protocol           = Protocol::OUCH;
            pkt.release_this_pkt   = false;

            return pkt;
        }();
        RxPacket ouch_bist_RxPacket_partial_2 = []() {
            RxPacket pkt{};

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
            pkt.len                = 134;
            pkt.venue              = Venue::BIST;
            pkt.protocol           = Protocol::OUCH;
            pkt.release_this_pkt   = false;

            return pkt;
        }();
        RxPacket ouch_bist_RxPacket_partial_3 = []() {
            RxPacket pkt{};

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
            pkt.len                = 78;
            pkt.venue              = Venue::BIST;
            pkt.protocol           = Protocol::OUCH;
            pkt.release_this_pkt   = false;

            return pkt;
        }();
        RxPacket ouch_bist_RxPacket_partial_4 = []() {
            RxPacket pkt{};

            const uint8_t data[] = {
            // Executed Message 
            0x00,                                               // Pkt length after this field (1st byte)
            };
            std::memcpy(pkt.data.data(), data, sizeof(data));
            pkt.len                = 1;
            pkt.venue              = Venue::BIST;
            pkt.protocol           = Protocol::OUCH;
            pkt.release_this_pkt   = false;

            return pkt;
        }();
        RxPacket ouch_bist_RxPacket_partial_5 = []() {
            RxPacket pkt{};

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
            pkt.len                = 70;
            pkt.venue              = Venue::BIST;
            pkt.protocol           = Protocol::OUCH;
            pkt.release_this_pkt   = false;

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
            0x41,                                           // 'A'
            0x00,0x00,0x00,0x10,                            // timestamp_ns = 16
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,        // order_id = 1
            0x00,0x00,0x00,0x02,                            // order_book_id = 2
            0x42,                                           // side = 'B'
            0x00,0x00,0x00,0x01,                            // ranking_seq_number = 1
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x64,        // quantity = 100
            0x00,0x00,0x00,0x32,                            // price = 50
            0x00,0x01,                                      // order_attributes
            0x02,                                           // lot_type
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10         // ranking_time
        };
        inline const std::vector<uint8_t> itch_bist_pkt3 = {
            0x45,                                           // 'E'
            0x00,0x00,0x00,0x20,                            // timestamp_ns
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,        // order_id
            0x00,0x00,0x00,0x02,                            // order_book_id
            0x53,                                           // side = 'S'
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x32,        // executed_quantity = 50
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x99,        // match_id
            0x00,0x00,0x00,0x03,                            // combo_group_id
            0,0,0,0,0,0,0,                                  // reserved1
            0,0,0,0,0,0,0                                   // reserved2
        };

        RxPacket itch_bist_RxPacket_single_1 = []() {
            RxPacket pkt{};

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
            pkt.len                = 130;
            pkt.venue              = Venue::BIST;
            pkt.protocol           = Protocol::ITCH;
            pkt.release_this_pkt   = false;

            return pkt;
        }();

        RxPacket itch_bist_RxPacket_full_1 = []() {
            RxPacket pkt{};

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
            'T','R','G','A','R','A','N','T','I','S','0','1',// isin (12 bytes) | offset: 73, len: 12
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
            
            0x41,                                           // 'A'
            0x00,0x00,0x00,0x10,                            // timestamp_ns = 16
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,        // order_id = 1
            0x00,0x00,0x00,0x02,                            // order_book_id = 2
            0x42,                                           // side = 'B'
            0x00,0x00,0x00,0x01,                            // ranking_seq_number = 1
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x64,        // quantity = 100
            0x00,0x00,0x00,0x32,                            // price = 50
            0x00,0x01,                                      // order_attributes
            0x02,                                           // lot_type
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,        // ranking_time

            0x45,                                           // 'E'
            0x00,0x00,0x00,0x20,                            // timestamp_ns
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,        // order_id
            0x00,0x00,0x00,0x02,                            // order_book_id
            0x53,                                           // side = 'S'
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x32,        // executed_quantity = 50
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x99,        // match_id
            0x00,0x00,0x00,0x03,                            // combo_group_id
            0,0,0,0,0,0,0,                                  // reserved1
            0,0,0,0,0,0,0                                   // reserved2

            };
            std::memcpy(pkt.data.data(), data, sizeof(data));
            pkt.len                = 227;
            pkt.venue              = Venue::BIST;
            pkt.protocol           = Protocol::ITCH;
            pkt.release_this_pkt   = false;

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

        RxPacket itch_nasdaq_RxPacket_single_1 = []() {
            RxPacket pkt{};

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
            pkt.len                = 39;
            pkt.venue              = Venue::NASDAQ;
            pkt.protocol           = Protocol::ITCH;
            pkt.release_this_pkt   = false;

            return pkt;
        }();

        RxPacket itch_nasdaq_RxPacket_full_1 = []() {
            RxPacket pkt{};

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

            0x41,                   // 'A'
            0x00,0x01,              // stock_locate = 1
            0x00,0x02,              // tracking_number = 2                                       
            0x00,0x00,0x00,0x00,0x00,0x64,  // timestamp (6 byte): 100
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01, // order_ref (8 byte)
            0x42,                   // side = 'B'
            0x00,0x00,0x00,0x64,    // shares 100
            0x41,0x41,0x50,0x4C,0x20,0x20,0x20,0x20, // stock "AAPL    "
            0x00,0x00,0x27,0x10,     // price 10000 (1.0000$)

            0x45,                   // 'E'
            0x00,0x01,              // stock_locate
            0x00,0x03,              // tracking_number
            0x00,0x00,0x00,0x00,0x00,0xC8, // timestamp 200
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01, // order_ref
            0x00,0x00,0x00,0x32,    // executed shares 50
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x99   // match_number
            };
            std::memcpy(pkt.data.data(), data, sizeof(data));
            pkt.len                = 106;
            pkt.venue              = Venue::NASDAQ;
            pkt.protocol           = Protocol::ITCH;
            pkt.release_this_pkt   = false;

            return pkt;
        }();

}