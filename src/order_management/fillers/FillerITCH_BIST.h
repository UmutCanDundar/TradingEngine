// This class is an implementation detail of OrderManager.
// It is split into a separate translation unit strictly for
// readability and protocol separation.
//
// This class is intentionally granted friend access to
// OrderManager to avoid exposing internal APIs or adding
// unnecessary indirection.
//
// The separation also anticipates a potential future refactor
// where protocol / venue pipelines may be fully decoupled
// (e.g. dedicated threads or execution paths per protocol-venue).
//
// A class-based design is preferred over namespace-level free functions
// to preserve a clear ownership boundary and execution context because
// these functions are not generic utilities, they are part of the OrderManager
// execution path.

#include "Protocol-Venue.h"

#pragma once

namespace BIST 
{
    struct ITCHAddOrderMessage;
    struct ITCHOrderExecutedMessage;
    struct ITCHOrderExecutedWithPriceMessage;
    struct ITCHOrderDeleteMessage;
    struct ITCHTradeMessage;
}

struct Order;
class OrderManager;

class FillerITCH_BIST
{
private:
    uint64_t bist_trade_id = 0;

    OrderManager &ord_mngr_;

public:
    FillerITCH_BIST(OrderManager &om) noexcept;

    void fill_itch_add(Order &order, const BIST::ITCHAddOrderMessage &msg, Venue venue) noexcept;
    void fill_itch_exec_report(Order &order, const BIST::ITCHOrderExecutedMessage &msg) noexcept;
    void fill_itch_exec_report(Order &order, const BIST::ITCHOrderExecutedWithPriceMessage &msg) noexcept;
    void fill_itch_delete(Order &order, const BIST::ITCHOrderDeleteMessage &msg) noexcept;
    void fill_itch_trade(Order &order, const BIST::ITCHTradeMessage &msg, Venue venue) noexcept;

};

