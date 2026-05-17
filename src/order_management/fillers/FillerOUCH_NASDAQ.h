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

namespace NASDAQ::RX
{
    struct OUCHOrderAcceptedMessage;
    struct OUCHOrderReplacedMessage;
    struct OUCHOrderCancelledMessage;
    struct OUCHOrderExecutedMessage;
    struct OUCHOrderRejectedMessage;
    struct OUCHOrderModifiedMessage;
}

struct Order;
class OrderManager;

class FillerOUCH_NASDAQ
{
private:
    OrderManager &ord_mngr_;

public:
    FillerOUCH_NASDAQ(OrderManager &om) noexcept;

    void fill_ouch_accepted(Order &order, const NASDAQ::RX::OUCHOrderAcceptedMessage &msg) noexcept;
    void fill_ouch_replaced(Order &order, const NASDAQ::RX::OUCHOrderReplacedMessage &msg) noexcept; // For now, only used for qty modification
    void fill_ouch_cancelled(Order &order, const NASDAQ::RX::OUCHOrderCancelledMessage &msg) noexcept;
    void fill_ouch_executed(Order &order, const NASDAQ::RX::OUCHOrderExecutedMessage &msg) noexcept;
    void fill_ouch_rejected(Order &order, const NASDAQ::RX::OUCHOrderRejectedMessage &msg) noexcept;
    void fill_ouch_modified(Order &order, const NASDAQ::RX::OUCHOrderModifiedMessage &msg) noexcept;
   
};

