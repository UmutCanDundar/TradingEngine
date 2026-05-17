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

namespace test_data_ordMngr {

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

    inline BIST::RX::OUCHOrderAcceptedMessage make_ouch_bist_accepted() {
        BIST::RX::OUCHOrderAcceptedMessage m{};
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
    inline BIST::RX::OUCHOrderExecutedMessage make_ouch_bist_executed() {
        BIST::RX::OUCHOrderExecutedMessage m{};
        m.message_type    = 'E';
        m.timestamp       = 200;        
        m.order_book_id   = 3;         
        m.traded_quantity = 60;       
        m.trade_price     = 10000;     
        m.client_category = 0;
        std::strncpy(m.order_token, "TOKEN000000042", sizeof(m.order_token));
        return m;
    }
    inline BIST::RX::OUCHOrderCancelledMessage make_ouch_bist_cancelled() {
        BIST::RX::OUCHOrderCancelledMessage m{};
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
    inline NASDAQ::RX::OUCHOrderAcceptedMessage make_ouch_nasdaq_accepted() {
        NASDAQ::RX::OUCHOrderAcceptedMessage m{};
        m.message_type              = 'A';
        m.timestamp                 = 100;      
        m.user_ref_num              = 99;      
        m.quantity                  = 1000;
        m.price                     = 10000;
        m.order_reference_number    = 99;      
        m.side                      = 'B';
        m.time_in_force             = '0';      
        std::strncpy(m.symbol,    "AAPL    ", sizeof(m.symbol));    // space-padded
        std::strncpy(m.cl_ord_id, "CLIENT0000001 ", sizeof(m.cl_ord_id));
        return m;
    }
    inline NASDAQ::RX::OUCHOrderExecutedMessage make_ouch_nasdaq_executed() {
        NASDAQ::RX::OUCHOrderExecutedMessage m{};
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
    inline NASDAQ::RX::OUCHOrderCancelledMessage make_ouch_nasdaq_cancelled() {
        NASDAQ::RX::OUCHOrderCancelledMessage m{};
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


}