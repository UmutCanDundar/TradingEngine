#include "FillerOUCH_NASDAQ.h"

#include "Order.h"
// #include "common.h"
#include "OrderManager.h"
#include "GeneratedOUCHMessages_BIST.h"

#include <cstdint>

FillerOUCH_NASDAQ::FillerOUCH_NASDAQ(OrderManager &om) noexcept : ord_mngr_(om) {}

void FillerOUCH_NASDAQ::fill_ouch_accepted(Order &order, const NASDAQ::OUT::OUCHOrderAcceptedMessage &msg) noexcept
{
    order.price = static_cast<int64_t>(msg.price);
    order.quantity = msg.quantity;
    order.remaining_quantity = msg.quantity;
    order.filled_quantity = order.quantity - order.remaining_quantity;
    order.status = Status::New;
    order.order_id = msg.order_reference_number;
    order.timestamp = msg.timestamp;
    order.last_update_time = order.timestamp;
}

void FillerOUCH_NASDAQ::fill_ouch_replaced(Order &order, const NASDAQ::OUT::OUCHOrderReplacedMessage &msg) noexcept // For now, only used for qty modification
{

    order.price = static_cast<int64_t>(msg.price);
    order.replaced_quantity = static_cast<int32_t>(msg.quantity - order.remaining_quantity);
    order.remaining_quantity = msg.quantity;
    order.quantity += order.replaced_quantity;
    order.status = Status::Replaced;
    order.order_id = msg.order_reference_number;
    order.timestamp = msg.timestamp;
    order.last_update_time = order.timestamp;
    order.side = msg.side == 'B' ? Side::Buy : Side::Sell;
    order.time_in_force = static_cast<TimeInForce>(msg.time_in_force);
    order.user_ref_num = msg.user_ref_num;
}

void FillerOUCH_NASDAQ::fill_ouch_cancelled(Order &order, const NASDAQ::OUT::OUCHOrderCancelledMessage &msg) noexcept
{
    order.last_update_time = order.timestamp;
    order.remaining_quantity = (order.remaining_quantity - msg.quantity > 0) ? order.remaining_quantity - msg.quantity : 0;
    order.replaced_quantity = msg.quantity;
    if (order.remaining_quantity > 0)
    {
        order.status = Status::Replaced;
        order.replaced_quantity *= -1;
    }
    else
    {
        order.status = Status::Cancelled;
    }
    order.cancelled_count++;
}

void FillerOUCH_NASDAQ::fill_ouch_executed(Order &order, const NASDAQ::OUT::OUCHOrderExecutedMessage &msg) noexcept
{
    order.price = static_cast<int64_t>(msg.price);
    order.filled_quantity += msg.quantity;
    order.last_exec_quantity = msg.quantity;
    order.last_update_time = msg.timestamp;
    order.remaining_quantity -= msg.quantity;
    if (order.filled_quantity < order.quantity)
        order.status = Status::Partial;
    else
        order.status = Status::Filled;
}

void FillerOUCH_NASDAQ::fill_ouch_rejected(Order &order, const NASDAQ::OUT::OUCHOrderRejectedMessage &msg) noexcept
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

void FillerOUCH_NASDAQ::fill_ouch_modified(Order &order, const NASDAQ::OUT::OUCHOrderModifiedMessage &msg) noexcept
{
    order.replaced_quantity = static_cast<int32_t>(msg.quantity - order.remaining_quantity);
    order.remaining_quantity = msg.quantity;
    order.quantity += order.replaced_quantity;
    order.status = Status::Replaced;
    order.side = msg.side == 'B' ? Side::Buy : Side::Sell;
    order.last_update_time = order.timestamp;
}