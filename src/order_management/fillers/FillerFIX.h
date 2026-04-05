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

struct FIXMessage;
struct Order;
class OrderManager;

class FillerFIX
{
private:
    OrderManager &ord_mngr_;

public:
    FillerFIX(OrderManager &om) noexcept; 

    void fill_fix_new(Order &order, const FIXMessage &msg) noexcept;
    void fill_fix_partial(Order &order, const FIXMessage &msg) noexcept;
    void fill_fix_filled(Order &order, const FIXMessage &msg) noexcept;
    void fill_fix_cancel(Order &order, const FIXMessage &msg) noexcept;
    void fill_fix_replaced(Order &order, const FIXMessage &msg) noexcept;
    void fill_fix_expired(Order &order, const FIXMessage &msg) noexcept;
    void fill_fix_cancel_reject(Order &order, const FIXMessage &msg) noexcept;
    void fill_fix_exec_report(Order &order, const FIXMessage &msg) noexcept;
  
};

