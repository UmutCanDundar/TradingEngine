
#include "FillerFIX.h"

#include "FIXMessage.h"
#include "Order.h"
#include "common.h"
#include "OrderManager.h"

#include <cstdint>
#include <string_view>

#include "absl/hash/hash.h"

FillerFIX::FillerFIX(OrderManager &om) noexcept : ord_mngr_(om) {}

void FillerFIX::fill_fix_new(Order &order, const FIXMessage &msg) noexcept
{
    order.price = msg.price;
    order.quantity = msg.quantity;
    order.remaining_quantity = msg.quantity;
    ord_mngr_.copy_order_id(order.fix_org_order_id, msg.order_id, FIXMessage::ID_SIZE);
    order.order_id = absl::Hash<std::string_view>{}(msg.order_id);
    order.timestamp = static_cast<uint64_t>(msg.transact_time);
    order.last_update_time = order.timestamp;
    order.order_type = static_cast<OrderType>(msg.ord_type - '0');
    order.status = Status::New;
    order.syncState = SyncState::NewSeen;
    order.StatusesPreNew.fill(Status::Unknown);
    order.side = (msg.side == 1) ? Side::Buy : Side::Sell;
    order.exec_type = msg.exec_type;
}

void FillerFIX::fill_fix_partial(Order &order, const FIXMessage &msg) noexcept
{
    order.status = Status::Partial;
    order.price = msg.last_price;
    order.filled_quantity = msg.filled_qty;
    order.last_exec_quantity = msg.last_qty;
    order.remaining_quantity = msg.leaves_qty;
    order.last_update_time = static_cast<uint64_t>(msg.transact_time);
    if (UNLIKELY(order.syncState == SyncState::WaitingNew))
        order.StatusesPreNew[0] = Status::Partial;
    order.exec_type = msg.exec_type;
}

void FillerFIX::fill_fix_filled(Order &order, const FIXMessage &msg) noexcept
{
    order.status = Status::Filled;
    order.price = msg.last_price;
    order.filled_quantity = msg.filled_qty;
    order.last_exec_quantity = msg.last_qty;
    order.remaining_quantity = msg.leaves_qty; // 0
    order.last_update_time = static_cast<uint64_t>(msg.transact_time);
    if (UNLIKELY(order.syncState == SyncState::WaitingNew))
        order.StatusesPreNew[0] = Status::Filled;
    order.exec_type = msg.exec_type;
}

void FillerFIX::fill_fix_cancel(Order &order, const FIXMessage &msg) noexcept
{
    order.status = Status::Deleted;
    order.filled_quantity = msg.filled_qty; // cum qty before cancel
    order.replaced_quantity = -1 * order.remaining_quantity;
    order.remaining_quantity = 0;
    order.last_update_time = static_cast<uint64_t>(msg.transact_time);
    if (UNLIKELY(order.syncState == SyncState::WaitingNew))
        order.StatusesPreNew[1] = Status::Deleted;
    order.cancelled_count++;
    order.exec_type = msg.exec_type;
}

void FillerFIX::fill_fix_replaced(Order &order, const FIXMessage &msg) noexcept
{
    order.price = msg.price;
    order.quantity = msg.quantity;
    order.replaced_quantity = msg.leaves_qty - order.remaining_quantity;
    order.remaining_quantity = msg.leaves_qty;
    ord_mngr_.copy_order_id(order.client_order_token, msg.cl_ord_id, FIXMessage::ID_SIZE);
    order.order_id = absl::Hash<std::string_view>{}(msg.order_id);
    order.last_update_time = static_cast<uint64_t>(msg.transact_time);
    order.status = Status::Replaced;
    if (UNLIKELY(order.syncState == SyncState::WaitingNew))
        order.StatusesPreNew[1] = Status::Replaced;
    order.side = (msg.side == 1) ? Side::Buy : Side::Sell;
    order.exec_type = msg.exec_type;

}

void FillerFIX::fill_fix_expired(Order &order, const FIXMessage &msg) noexcept
{
    order.status = Status::Expired;
    order.filled_quantity = msg.filled_qty;
    order.last_exec_quantity = msg.last_qty;
    order.replaced_quantity = -1 * order.remaining_quantity;
    order.remaining_quantity = 0;
    order.last_update_time = static_cast<uint64_t>(msg.transact_time);
    if (UNLIKELY(order.syncState == SyncState::WaitingNew))
        order.StatusesPreNew[1] = Status::Expired;
    order.exec_type = msg.exec_type;
}

void FillerFIX::fill_fix_cancel_reject(Order &order, const FIXMessage &msg) noexcept
{
    order.last_update_time = static_cast<uint64_t>(msg.transact_time);
}

void FillerFIX::fill_fix_exec_report(Order &order, const FIXMessage &msg) noexcept
{
    switch (msg.ord_status)
    {
    case '0': 
        fill_fix_new(order, msg);
        break;
    case '1': 
        fill_fix_partial(order, msg);
        break;
    case '2': 
        fill_fix_filled(order, msg);
        break;
    case '4': 
        fill_fix_cancel(order, msg);
        break;
    case '5': 
        fill_fix_replaced(order, msg);
        break;
    case 'C': 
        fill_fix_expired(order, msg);
        break;
    }
}