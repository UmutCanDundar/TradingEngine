#include "dataset.h"


namespace test_data {

//============= FIX DATA ==================

const std::vector<uint8_t> single_fix_pkt1 = {
    // -------- New Order (D) PARTIAL --------
    '8','=','F','I','X','.','4','.','2','\x01',
    '9','=','0','6','3','\x01',          // BodyLength 
    '3','5','=','D','\x01',
    '3','4','=','1','\x01',              // seqnum
    '4','9','=','C','L','I','E','N','T','\x01',
    '5','6','=','E','X','C','H','\x01',
    '1','1','=','O','1','\x01',          // cl_ord_id
    '5','5','=','G','A','R','A','N','\x01',
    '3','8','=','1','0','0','\x01',      // qty
    '4','4','=','1','0','0','0','\x01',  // price
    '5','4','=','1','\x01',              // side (BUY)
    '1','0','=','1','6','4','\x01',      // checksum
};
const std::vector<uint8_t> single_fix_pkt2 = {
    // -------- Execution Report --------
    '8','=','F','I','X','.','4','.','2','\x01',
    '9','=','0','4','2','\x01',
    '3','5','=','8','\x01',
    '3','4','=','3','\x01',
    '3','9','=','2','\x01',              // FILLED
    '1','5','0','=','F','\x01',          // exec_type
    '1','1','=','O','1','\x01',
    '3','2','=','1','0','0','\x01',      // last_qty
    '3','1','=','1','0','0','0','\x01',  // last_price
    '1','0','=','0','9','5','\x01',      // checksum
};

OutPacket fix_outpacket = []() {
    OutPacket pkt{};

    const uint8_t data[] = {
        // ── New Order (D) ── offset: 0
        '8','=','F','I','X','.','4','.','2','\x01',
        '9','=','0','6','3','\x01',
        '3','5','=','D','\x01',
        '3','4','=','1','\x01',
        '4','9','=','C','L','I','E','N','T','\x01',
        '5','6','=','E','X','C','H','\x01',
        '1','1','=','O','1','\x01',
        '5','5','=','G','A','R','A','N','\x01',
        '3','8','=','1','0','0','\x01',
        '4','4','=','1','0','0','0','\x01',
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
        '9','=','0','4','2','\x01',
        '3','5','=','8','\x01',
        '3','4','=','3','\x01',
        '3','9','=','2','\x01',
        '1','5','0','=','F','\x01',
        '1','1','=','O','1','\x01',
        '3','2','=','1','0','0','\x01',
        '3','1','=','1','0','0','0','\x01',
        '1','0','=','0','9','5','\x01',
    };

    std::memcpy(pkt.data.data(), data, sizeof(data));
    pkt.len        = 205;
    pkt.consumed    = false;
    pkt.venue      = Venue::BIST;
    pkt.protocol   = Protocol::FIX;

    return pkt;
}();

OutPacket fix_outpacket_partial_1 = []() {
    OutPacket pkt{};

    const uint8_t data[] = {
        // ── New Order (D) ── offset: 0
        '8','=','F','I','X','.','4','.','2','\x01',
        '9','=','0','6','3','\x01',
        '3','5','=','D','\x01',
        '3','4','=','1','\x01',
        '4','9','=','C','L','I','E','N','T','\x01',
        '5','6','=','E','X','C','H','\x01',
        '1','1','=','O','1','\x01',
        '5','5','=','G','A','R','A','N','\x01',
        '3','8','=','1','0','0','\x01',
        '4','4','=','1','0','0','0','\x01',
        '5','4','=','1','\x01',
        '1','0','=','1',
    };

    std::memcpy(pkt.data.data(), data, sizeof(data));
    pkt.len        = 83;
    pkt.consumed   = false;
    pkt.venue      = Venue::BIST;
    pkt.protocol   = Protocol::FIX;

    return pkt;
}();

OutPacket fix_outpacket_partial_2 = []() {
    OutPacket pkt{};

    const uint8_t data[] = {
        '6','4','\x01',
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
        '9','=','0','4','2','\x01',
    };

    std::memcpy(pkt.data.data(), data, sizeof(data));
    pkt.len        = 73;
    pkt.consumed   = false;
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
        '3','1','=','1','0','0','0','\x01',
        '1','0','=','0','9','5','\x01',
    };

    std::memcpy(pkt.data.data(), data, sizeof(data));
    pkt.len        = 49;
    pkt.consumed   = false;
    pkt.venue      = Venue::BIST;
    pkt.protocol   = Protocol::FIX;

    return pkt;
}();

OutPacket fix_outpacket_partial_4 = []() {
    OutPacket pkt{};

    const uint8_t data[] = {
        // ── New Order (D) ── offset: 0
        '8','=','F','I','X','.','4','.','2','\x01',
        '9','=','0','6','3','\x01',
        '3','5','=','D','\x01',
        '3','4','=','2','\x01',
        '4','9','=','C','L','I','E','N','T','\x01',
        '5','6','=','E','X','C','H','\x01',
        '1','1','=','O','1','\x01',
        '5','5','=','G','A','R','A','N','\x01',
    };

    std::memcpy(pkt.data.data(), data, sizeof(data));
    pkt.len        = 59;
    pkt.consumed   = false;
    pkt.venue      = Venue::BIST;
    pkt.protocol   = Protocol::FIX;

    return pkt;
}();

OutPacket fix_outpacket_partial_5 = []() {
    OutPacket pkt{};

    const uint8_t data[] = {
        '3','8','=','1','0','0','\x01',
        '4','4','=','1','0','0','0','\x01',
    };

    std::memcpy(pkt.data.data(), data, sizeof(data));
    pkt.len        = 15;
    pkt.consumed   = false;
    pkt.venue      = Venue::BIST;
    pkt.protocol   = Protocol::FIX;

    return pkt;
}();

OutPacket fix_outpacket_partial_6 = []() {
    OutPacket pkt{};

    const uint8_t data[] = {
        '5','4','=','1','\x01',
        '1','0','=','1','6','4','\x01',
         // ── Cancel (F) ── offset: 86
        '8','=','F','I','X','.','4','.','2','\x01',
        '9','=','0','3','1','\x01',
        '3','5','=','F','\x01',
        '3','4','=','3','\x01',
        '1','1','=','O','2','\x01',
        '4','1','=','O','1','\x01',
        '5','5','=','T','U','P','R','S','\x01',
        '1','0','=','0','7','1','\x01',
        // ── Execution Report (8) ── offset: 140
        '8','=','F','I','X','.','4','.','2','\x01',
        '9','=','0','4','2','\x01',
        '3','5','=','8','\x01',
        '3','4','=','1','\x01',
        '3','9','=','2','\x01',
        '1','5','0','=','F','\x01',
        '1','1','=','O','1','\x01',
        '3','2','=','1','0','0','\x01',
        '3','1','=','1','0','0','0','\x01',
        '1','0','=','0','9','5','\x01',
    };

    std::memcpy(pkt.data.data(), data, sizeof(data));
    pkt.len        = 131;
    pkt.consumed   = false;
    pkt.venue      = Venue::BIST;
    pkt.protocol   = Protocol::FIX;

    return pkt;
}();

//============= OUCH DATA ==================

const std::vector<uint8_t> ouch_bist_pkt1 = {
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

const std::vector<uint8_t> ouch_bist_pkt2 = {
    'C',                                                 // offset: 0
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,            // timestamp | offset: 1
    'T','O','K','E','N','0','0','0','0','0','0','0','0','2',  // order_token | offset: 9
    0x00,0x00,0x00,0x02,                                 // order_book_id | offset: 23
    'B',                                                 // side | offset: 27
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,            // order_id | offset: 28
    0x00,                                                // reason | offset: 36
};

const std::vector<uint8_t> ouch_bist_pkt3 = {
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

const std::vector<uint8_t> ouch_nasdaq_pkt1 = {
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

const std::vector<uint8_t> ouch_nasdaq_pkt2 = {
    'C',                                                // message_type | offset: 0
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,           // timestamp | offset: 1
    0x00,0x00,0x00,0x02,                                // user_ref_num | offset: 9
    0x00,0x00,0x01,0xF4,                                // quantity | offset: 13
    'U',                                                // reason | offset: 17
    0x00,0x00,                                          // appendage_length | offset: 18
};

const std::vector<uint8_t> ouch_nasdaq_pkt3 = {
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

//============= ITCH DATA ==================

const std::vector<uint8_t> itch_bist_pkt1 = {
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
const std::vector<uint8_t> itch_bist_pkt2 = {
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
const std::vector<uint8_t> itch_bist_pkt3 = {
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


const std::vector<uint8_t> itch_nasdaq_pkt1 = {
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
const std::vector<uint8_t> itch_nasdaq_pkt2 = {
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
const std::vector<uint8_t> itch_nasdaq_pkt3 = {
    0x45,                   // 'E'
    0x00,0x01,              // stock_locate
    0x00,0x03,              // tracking_number
    0x00,0x00,0x00,0x00,0x00,0xC8, // timestamp 200
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01, // order_ref
    0x00,0x00,0x00,0x32,    // executed shares 50
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x99   // match_number
};


}






// const std::vector<uint8_t> ouch_bist_pkt1 = {
//     // -------- O (Enter Order) --------
//     'O',
//     'T','O','K','E','N','0','0','0','0','0','0','0','1', // order_token[14]
//     0x00,0x00,0x00,0x01, // order_book_id
//     'B', // side
//     0x00,0x00,0x00,0x00,0x00,0x00,0x03,0xE8, // quantity = 1000
//     0x00,0x00,0x27,0x10, // price = 10000
//     0x00, // time_in_force
//     0x00, // open_close

//     // -------- X (Cancel) --------
//     'X',
//     'T','O','K','E','N','0','0','0','0','0','0','0','1',

//     // -------- u (Replace) PARTIAL --------
//     'u',
//     'T','O','K','E','N','0','0','0','0','0','0','0','1', // existing
//     'T','O','K','E','N','0','0','0','0','0','0','0','2'  // replacement
//     // CUT HERE → partial
// };
// const std::vector<uint8_t> ouch_bist_pkt2 = {
//     // -------- u REST --------
//     0x00,0x00,0x00,0x00,0x00,0x00,0x07,0xD0, // quantity = 2000
//     0x00,0x00,0x30,0x39, // price = 12345

//     // -------- A (Accepted) --------
//     'A',
//     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01, // timestamp
//     'T','O','K','E','N','0','0','0','0','0','0','0','2',
//     0x00,0x00,0x00,0x02, // order_book_id
//     'B',
//     0x00,0x00,0x00,0x00,0x00,0x00,0x07,0xD0, // qty

//     // -------- C (Cancelled) --------
//     'C',
//     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,
//     'T','O','K','E','N','0','0','0','0','0','0','0','2',
//     0x00 // reason
// };
// const std::vector<uint8_t> ouch_bist_pkt3 = {
//     // -------- E (Executed) --------
//     'E',
//     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,
//     'T','O','K','E','N','0','0','0','0','0','0','0','2',
//     0x00,0x00,0x00,0x00,0x00,0x00,0x03,0xE8, // traded qty
//     0x00,0x00,0x27,0x10, // price

//     // -------- X --------
//     'X',
//     'T','O','K','E','N','0','0','0','0','0','0','0','2',

//     // -------- O --------
//     'O',
//     'T','O','K','E','N','0','0','0','0','0','0','0','3',
//     0x00,0x00,0x00,0x03,
//     'S',
//     0x00,0x00,0x00,0x00,0x00,0x00,0x01,0xF4, // qty=500
//     0x00,0x00,0x13,0x88 // price=5000
// };

// const std::vector<uint8_t> ouch_nasdaq_pkt1 = {
//  // -------- A (Order Accepted) --------
//     'A',
//     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01, // timestamp
//     0x00,0x00,0x00,0x01,                     // user_ref_num
//     0x00,0x00,0x03,0xE8,                     // quantity = 1000
//     'B',                                     // side
//     'A','A','P','L',' ',' ',' ',' ',         // symbol[8]
//     0x00,0x00,0x00,0x00,0x00,0x00,0x27,0x10, // price = 10000
//     '0',                                     // tif
//     'Y',                                     // display
//     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02, // order_reference_number
//     'A',                                     // capacity
//     'N',                                     // iso
//     'N',                                     // cross
//     'L',                                     // order_state
//     'C','L','I','E','N','T','0','0','0','0','0','0','1',' ', // cl_ord_id

//     // -------- E (Executed) --------
//     'E',
//     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02, // timestamp
//     0x00,0x00,0x00,0x01,                     // user_ref_num
//     0x00,0x00,0x01,0xF4,                     // qty = 500
// };
// const std::vector<uint8_t> ouch_nasdaq_pkt2 = {
//     0x00,0x00,0x00,0x00,0x00,0x00,0x27,0x10, // price
//     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03, // match_number
//     'A',                                     // liquidity_flag

//     // -------- U (Order Replaced) --------
//     'U',
//     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03, // timestamp
//     0x00,0x00,0x00,0x01,                     // orig_user_ref_num
//     0x00,0x00,0x00,0x02,                     // new user_ref_num
//     0x00,0x00,0x03,0xE8,                     // qty = 1000
//     'B',
//     'A','A','P','L',' ',' ',' ',' ',
//     0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x39, // price = 12345
//     '0',
//     'L',
//     'C','L','I','E','N','T','0','0','0','0','0','0','2',' ',

//     // -------- C (Cancelled) --------
//     'C',
//     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04, // timestamp
//     0x00,0x00,0x00,0x02,                     // user_ref_num
//     0x00,0x00,0x01,0xF4,                     // cancelled qty = 500
//     'U'                                      // reason
// };
// const std::vector<uint8_t> ouch_nasdaq_pkt3 = {
//     // -------- J (Rejected) --------
//     'J',
//     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x05, // timestamp
//     0x00,0x00,0x00,0x03,                     // user_ref_num
//     0x00,0x01,                               // reason
//     'C','L','I','E','N','T','0','0','0','0','0','0','3',' ',

//     // -------- B (Broken Trade) --------
//     'B',
//     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06, // timestamp
//     0x00,0x00,0x00,0x03,                     // user_ref_num
//     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04, // match_number
//     'E',                                     // reason
//     'C','L','I','E','N','T','0','0','0','0','0','0','4',' ',

//     // -------- S (System Event) --------
//     'S',
//     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x07, // timestamp
//     'S'                                      // Start of Day
// };