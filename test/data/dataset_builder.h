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

namespace test_data_builder {

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
        std::strncpy(o->client_order_token.data(), "CLIENT0000001", 14);
        o->real_cl_ord_token_len = 13;
        o->client_order_id = absl::Hash<std::string_view>{}("CLIENT0000001");
        
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
        std::strncpy(o->client_order_token.data(), "CLIENT0000005", 13);
        o->real_cl_ord_token_len = 13;
        o->client_order_id = absl::Hash<std::string_view>{}("CLIENT0000005 ");
        
        return o;
    }
    inline auto make_ouch_replace_order()
    {
        auto* o = new Order();
        o->price              = 20000;
        o->quantity           = 200;
        o->side               = Side::Buy;
        o->venue              = Venue::BIST;
        o->protocol           = Protocol::OUCH;
        o->instrument_id      = 3;
        o->symbol_index       = 0;
        o->order_type         = OrderType::Limit;
        o->time_in_force      = TimeInForce::DAY;
        o->message_type       = 1;
        o->isOurOrder         = true;
        o->account_index      = 0;
        o->timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        std::strncpy(o->client_order_token.data(), "CLIENT0000002", 13);
        o->real_cl_ord_token_len = 13;
        std::strncpy(o->fix_org_order_id.data(), "ORGORDER00001", 13);
        o->client_order_id = absl::Hash<std::string_view>{}("CLIENT0000002 ");
        return o;
    }

    inline auto make_ouch_cancel_order()
    {
        auto* o = new Order();
        o->venue              = Venue::BIST;
        o->protocol           = Protocol::OUCH;
        o->message_type       = 2;
        o->isOurOrder         = true;
        o->account_index      = 0;
        o->timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        std::strncpy(o->client_order_token.data(), "CLIENT0000001", 13);
        o->real_cl_ord_token_len = 13;
        o->client_order_id = absl::Hash<std::string_view>{}("CLIENT0000001 ");
        return o;
    }

    inline auto make_ouch_cancelid_order()
    {
        auto* o = new Order();
        o->venue              = Venue::BIST;
        o->protocol           = Protocol::OUCH;
        o->instrument_id      = 3;
        o->side               = Side::Buy;
        o->order_id           = 123456789;
        o->message_type       = 3;
        o->isOurOrder         = true;
        o->account_index      = 0;
        o->timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        o->client_order_id = absl::Hash<std::string_view>{}("CANCELID00001 ");
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
        std::strncpy(o->client_order_token.data(), "CLIENT0000008", 13);
        o->real_cl_ord_token_len = 13;
        o->client_order_id = absl::Hash<std::string_view>{}("CLIENT0000008 ");
        
        return o;
    }
    inline auto make_NQouch_replace_order()
    {
        auto* o = new Order();
        o->price              = 20000;
        o->quantity           = 200;
        o->side               = Side::Sell;
        o->venue              = Venue::NASDAQ;
        o->protocol           = Protocol::OUCH;
        o->message_type       = 1;
        o->isOurOrder         = true;
        o->time_in_force      = TimeInForce::DAY;
        o->timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        std::strncpy(o->client_order_token.data(), "CLIENT0000002", 13);
        o->real_cl_ord_token_len = 13;
        o->pre_user_ref_num   = 1;
        o->user_ref_num       = 2;
        o->client_order_id = absl::Hash<std::string_view>{}("CLIENT0000002 ");
        return o;
    }

    inline auto make_NQouch_cancel_order()
    {
        auto* o = new Order();
        o->venue              = Venue::NASDAQ;
        o->protocol           = Protocol::OUCH;
        o->quantity           = 100;
        o->message_type       = 2;
        o->isOurOrder         = true;
        o->timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        o->user_ref_num       = 1;
        o->client_order_id = absl::Hash<std::string_view>{}("NQCANCEL00001 ");
        return o;
    }

    inline auto make_NQouch_modify_order()
    {
        auto* o = new Order();
        o->venue              = Venue::NASDAQ;
        o->protocol           = Protocol::OUCH;
        o->quantity           = 150;
        o->side               = Side::Sell;
        o->message_type       = 3;
        o->isOurOrder         = true;
        o->timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        o->user_ref_num       = 1;
        o->client_order_id = absl::Hash<std::string_view>{}("NQMODIFY00001 ");
        return o;
    }

    inline auto* fix_new_order        = make_fix_order(); 
    
    inline auto* ouch_new_order       = make_ouch_new_order();
    inline auto* ouch_replace_order   = make_ouch_replace_order();
    inline auto* ouch_cancel_order    = make_ouch_cancel_order();
    inline auto* ouch_cancelid_order  = make_ouch_cancelid_order();

    inline auto* NQouch_new_order     = make_NQouch_new_order();
    inline auto* NQouch_replace_order = make_NQouch_replace_order();
    inline auto* NQouch_cancel_order  = make_NQouch_cancel_order();
    inline auto* NQouch_modify_order  = make_NQouch_modify_order();

    inline auto make_fix_order2() // For engine_Tx_bench test
    {
        auto* o = new Order(); 
        o->price           = 10000;
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
        std::strncpy(o->client_order_token.data(), "CLIENT0000001", 14);
        o->real_cl_ord_token_len = 13;
        o->client_order_id = absl::Hash<std::string_view>{}("CLIENT0000001");
        
        return o;
    }

    inline auto* fix_new_order2        = make_fix_order2(); 
}