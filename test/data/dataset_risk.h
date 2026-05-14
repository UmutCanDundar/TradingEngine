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

namespace test_data_risk {

//===========================================================================
//                  ORDER MANAGER TO RISK RISK TEST DATA
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

}