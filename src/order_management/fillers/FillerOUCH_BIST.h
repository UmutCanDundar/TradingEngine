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

#pragma once

namespace BIST::OUT
{
    struct OUCHOrderAcceptedMessage;
    struct OUCHOrderReplacedMessage;
    struct OUCHOrderCancelledMessage;
    struct OUCHOrderExecutedMessage;
    struct OUCHOrderRejectedMessage;
}

struct Order;
class OrderManager;

class FillerOUCH_BIST
{
private:
    OrderManager &ord_mngr_;

public:
    FillerOUCH_BIST(OrderManager &om) noexcept;

    void fill_ouch_accepted(Order &order, const BIST::OUT::OUCHOrderAcceptedMessage &msg) noexcept;
    void fill_ouch_replaced(Order &order, const BIST::OUT::OUCHOrderReplacedMessage &msg) noexcept;
    void fill_ouch_cancelled(Order &order, const BIST::OUT::OUCHOrderCancelledMessage &msg) noexcept;
    void fill_ouch_executed(Order &order, const BIST::OUT::OUCHOrderExecutedMessage &msg) noexcept;
    void fill_ouch_rejected(Order &order, const BIST::OUT::OUCHOrderRejectedMessage &msg) noexcept;
   
};

