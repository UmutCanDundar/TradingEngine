#include "FillerITCH_NASDAQ.h"

#include "Order.h"
#include "common.h"
#include "OrderManager.h"
#include "GeneratedITCHMessages_NASDAQ.h"
#include "HashTables.h"

#include <cstdint>
#include <cstddef>

FillerITCH_NASDAQ::FillerITCH_NASDAQ(OrderManager &om) noexcept : ord_mngr_(om) {}

void FillerITCH_NASDAQ::fill_itch_add(Order &order, const NASDAQ::ITCHAddOrderMessage &msg, Venue venue) noexcept
{

    order.price = static_cast<int64_t>(msg.price);
    order.quantity = msg.shares;
    order.remaining_quantity = msg.shares;
    order.instrument_id = msg.stock_locate;

    auto *symbolmeta = ord_mngr_.get_symbolmeta(venue, order.instrument_id);
    if (symbolmeta)
    {
        order.symbol = symbolmeta->symbol;
        order.symbol_index = ord_mngr_.hashtables_.getIndex(static_cast<size_t>(venue), order.symbol);
    }
    else
    {
        ord_mngr_.release_order(order);
        return;
    }

    order.side = (msg.side == 'B') ? Side::Buy : Side::Sell;
    order.status = Status::New;
    order.order_id = msg.order_ref;
    order.timestamp = msg.timestamp;
    order.last_update_time = order.timestamp;
    order.message_type = msg.message_type;
    order.protocol = Protocol::ITCH;
    order.venue = venue;
    order.order_type = (order.price < 0) ? OrderType::Limit : OrderType::Market;
}

void FillerITCH_NASDAQ::fill_itch_add(Order &order, const NASDAQ::ITCHAddOrderMPIDMessage &msg, Venue venue) noexcept
{
    order.price = static_cast<int64_t>(msg.price);
    order.quantity = msg.shares;
    order.instrument_id = msg.stock_locate;
    order.remaining_quantity = msg.shares;
    ord_mngr_.copy_symbol(order.symbol, msg.stock);

    auto *symbolmeta = ord_mngr_.get_symbolmeta(venue, order.instrument_id);
    if (symbolmeta)
    {
        order.symbol = symbolmeta->symbol;
        order.symbol_index = ord_mngr_.hashtables_.getIndex(static_cast<size_t>(venue), order.symbol);
    }
    else
    {
        ord_mngr_.release_order(order);
        return;
    }

    order.side = (msg.side == 'B') ? Side::Buy : Side::Sell;
    order.status = Status::New;
    order.order_id = msg.order_ref;
    order.timestamp = msg.timestamp;
    order.last_update_time = order.timestamp;
    order.message_type = msg.message_type;
    order.protocol = Protocol::ITCH;
    order.venue = venue;
    order.order_type = (order.price < 0) ? OrderType::Limit : OrderType::Market;
}

void FillerITCH_NASDAQ::fill_itch_cancel(Order &order, const NASDAQ::ITCHCancelMessage &msg) noexcept
{

    order.last_update_time = msg.timestamp;
    order.remaining_quantity = order.remaining_quantity - msg.cancelled_shares;
    order.replaced_quantity = msg.cancelled_shares;
    order.status = Status::Cancelled;
    order.cancelled_count++;
    order.message_type = msg.message_type;
}

void FillerITCH_NASDAQ::fill_itch_exec_report(Order &order, const NASDAQ::ITCHExecutedMessage &msg) noexcept
{

    order.last_update_time = msg.timestamp;
    order.filled_quantity += msg.executed_shares;
    order.last_exec_quantity = msg.executed_shares;
    order.remaining_quantity -= msg.executed_shares;
    if (order.remaining_quantity > 0)
        order.status = Status::Partial;
    else
        order.status = Status::Filled;

    order.message_type = msg.message_type;
}

void FillerITCH_NASDAQ::fill_itch_exec_report(Order &order, const NASDAQ::ITCHExecutedWithPriceMessage &msg) noexcept
{
    fill_itch_exec_report(order, *reinterpret_cast<const NASDAQ::ITCHExecutedMessage *>(&msg));
    order.price = static_cast<int64_t>(msg.execution_price);
}

void FillerITCH_NASDAQ::fill_itch_delete(Order &order, const NASDAQ::ITCHDeleteMessage &msg) noexcept
{

    order.status = Status::Deleted;
    order.last_update_time = msg.timestamp;
    order.message_type = msg.message_type;
    order.cancelled_count++;
}

void FillerITCH_NASDAQ::fill_itch_replace(Order &order, const NASDAQ::ITCHReplaceMessage &msg, Venue venue) noexcept
{
    order.order_id = msg.new_order_ref;
    order.price = msg.price;
    order.quantity = msg.shares;
    order.instrument_id = msg.stock_locate;
    order.filled_quantity = 0;
    order.remaining_quantity = msg.shares;

    auto *symbolmeta = ord_mngr_.get_symbolmeta(venue, order.instrument_id);
    if (symbolmeta)
    {
        order.symbol = symbolmeta->symbol;
        order.symbol_index = ord_mngr_.hashtables_.getIndex(static_cast<size_t>(venue), order.symbol);
    }
    else
    {
        ord_mngr_.release_order(order);
        return;
    }

    order.message_type = msg.message_type;
    order.status = Status::Replaced;
    order.protocol = Protocol::ITCH;
    order.venue = venue;
    order.last_update_time = msg.timestamp;
    order.order_type = (order.price < 0) ? OrderType::Limit : OrderType::Market;
}

void FillerITCH_NASDAQ::fill_itch_trade(Order &order, const NASDAQ::ITCHTradeMessage &msg, Venue venue) noexcept
{
    if (UNLIKELY(order.isOurOrder))
        return;

    order.order_id = msg.order_ref;
    order.filled_quantity += msg.shares;
    order.last_exec_quantity = msg.shares;
    order.last_update_time = msg.timestamp;
    order.remaining_quantity = 0;
    order.instrument_id = msg.stock_locate;

    auto *symbolmeta = ord_mngr_.get_symbolmeta(venue, order.instrument_id);
    if (symbolmeta)
    {
        order.symbol = symbolmeta->symbol;
        order.symbol_index = ord_mngr_.hashtables_.getIndex(static_cast<size_t>(venue), order.symbol);
    }
    else
    {
        ord_mngr_.release_order(order);
        return;
    }

    order.price = static_cast<int64_t>(msg.price);
    order.status = Status::Filled;
    order.protocol = Protocol::ITCH;
    order.venue = venue;
    order.message_type = msg.message_type;
}