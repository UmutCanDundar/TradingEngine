#include "FillerOUCH_BIST.h"

#include "Order.h"
//#include "common.h"
#include "OrderManager.h"
#include "GeneratedOUCHMessages_BIST.h"

#include <cstdint>

FillerOUCH_BIST::FillerOUCH_BIST(OrderManager &om) noexcept : ord_mngr_(om) {}

void FillerOUCH_BIST::fill_ouch_accepted(Order &order, const BIST::OUT::OUCHOrderAcceptedMessage &msg) noexcept
{

    order.price = static_cast<int64_t>(msg.price);
    order.quantity = static_cast<uint32_t>(msg.quantity);
    order.remaining_quantity = static_cast<uint32_t>(msg.quantity);
    order.status = Status::New;
    order.order_id = msg.order_id;
    order.timestamp = msg.timestamp;
    order.last_update_time = order.timestamp;
}

void FillerOUCH_BIST::fill_ouch_replaced(Order &order, const BIST::OUT::OUCHOrderReplacedMessage &msg) noexcept
{
    if (msg.order_state == 2)
        return;

    order.price = static_cast<int64_t>(msg.price);
    order.replaced_quantity = static_cast<int32_t>(msg.quantity - order.remaining_quantity); 
    order.remaining_quantity = static_cast<uint32_t>(msg.quantity);                          
    order.quantity += order.replaced_quantity;                                              
    order.status = Status::Replaced;
    order.order_id = msg.order_id;
    order.timestamp = msg.timestamp;
    order.last_update_time = order.timestamp;
    order.side = msg.side == 'B' ? Side::Buy : Side::Sell;
    order.time_in_force = static_cast<TimeInForce>(msg.time_in_force);
}

void FillerOUCH_BIST::fill_ouch_cancelled(Order &order, const BIST::OUT::OUCHOrderCancelledMessage &msg) noexcept
{

    order.last_update_time = msg.timestamp;
    order.replaced_quantity = -1 * order.remaining_quantity;
    order.remaining_quantity = 0;
    order.status = Status::Deleted;
    order.cancelled_count++;
}
void FillerOUCH_BIST::fill_ouch_executed(Order &order, const BIST::OUT::OUCHOrderExecutedMessage &msg) noexcept
{

    order.price = static_cast<int64_t>(msg.trade_price);
    order.filled_quantity += msg.traded_quantity;
    order.last_exec_quantity = msg.traded_quantity;
    order.last_update_time = msg.timestamp;
    order.remaining_quantity -= msg.traded_quantity;
    if (order.filled_quantity < order.quantity)
        order.status = Status::Partial;
    else
        order.status = Status::Filled;
}

void FillerOUCH_BIST::fill_ouch_rejected(Order &order, const BIST::OUT::OUCHOrderRejectedMessage &msg) noexcept
{
    if (order.status == Status::Unknown)
    {
        ord_mngr_.store_to_strategy_free_slot_.push(&order);
    }
    else
    {
        order.last_update_time = msg.timestamp;
        order.canModify = 0x00;
    }
}