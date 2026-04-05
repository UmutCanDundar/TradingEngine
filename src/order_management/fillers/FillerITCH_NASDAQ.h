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

namespace NASDAQ
{
    struct ITCHAddOrderMessage;
    struct ITCHAddOrderMPIDMessage;
    struct ITCHCancelMessage;
    struct ITCHExecutedMessage;
    struct ITCHExecutedWithPriceMessage;
    struct ITCHDeleteMessage;
    struct ITCHReplaceMessage;
    struct ITCHTradeMessage;
}

struct Order;
class OrderManager;

class FillerITCH_NASDAQ
{
private:
    OrderManager &ord_mngr_;

public:
    FillerITCH_NASDAQ(OrderManager &om) noexcept;

    void fill_itch_add(Order &order, const NASDAQ::ITCHAddOrderMessage &msg, Venue venue) noexcept;
    void fill_itch_add(Order &order, const NASDAQ::ITCHAddOrderMPIDMessage &msg, Venue venue) noexcept;
    void fill_itch_cancel(Order &order, const NASDAQ::ITCHCancelMessage &msg) noexcept;
    void fill_itch_exec_report(Order &order, const NASDAQ::ITCHExecutedMessage &msg) noexcept;
    void fill_itch_exec_report(Order &order, const NASDAQ::ITCHExecutedWithPriceMessage &msg) noexcept;
    void fill_itch_delete(Order &order, const NASDAQ::ITCHDeleteMessage &msg) noexcept;
    void fill_itch_replace(Order &order, const NASDAQ::ITCHReplaceMessage &msg, Venue venue) noexcept;
    void fill_itch_trade(Order &order, const NASDAQ::ITCHTradeMessage &msg, Venue venue) noexcept;
    
};

