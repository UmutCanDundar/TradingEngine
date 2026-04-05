#include "FillerITCH_BIST.h"

#include "Order.h"
#include "common.h"
#include "OrderManager.h"
#include "GeneratedITCHMessages_BIST.h"
#include "HashTables.h"

#include <cstdint>

FillerITCH_BIST::FillerITCH_BIST(OrderManager &om) noexcept : ord_mngr_(om) {}

void FillerITCH_BIST::fill_itch_add(Order &order, const BIST::ITCHAddOrderMessage &msg, Venue venue) noexcept
{
    order.price = static_cast<int64_t>(msg.price);
    order.quantity = static_cast<uint32_t>(msg.quantity);
    order.remaining_quantity = static_cast<uint32_t>(msg.quantity);
    order.instrument_id = msg.order_book_id;
    order.side = (msg.side == 'B') ? Side::Buy : Side::Sell;
    order.order_id = msg.order_id;

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

    order.status = Status::New;
    order.timestamp = msg.timestamp_ns;
    order.last_update_time = order.timestamp;
    order.message_type = msg.message_type;
    order.protocol = Protocol::ITCH;
    order.order_type = (order.price < 0) ? OrderType::Limit : OrderType::Market;
    order.cancelled_count = 0;
}

void FillerITCH_BIST::fill_itch_exec_report(Order &order, const BIST::ITCHOrderExecutedMessage &msg) noexcept
{
    order.filled_quantity += msg.executed_quantity;
    order.last_exec_quantity = msg.executed_quantity;
    order.remaining_quantity -= msg.executed_quantity;
    order.last_update_time = msg.timestamp_ns;
    if (order.remaining_quantity > 0)
        order.status = Status::Partial;
    else
        order.status = Status::Filled;

    order.message_type = msg.message_type;
}

void FillerITCH_BIST::fill_itch_exec_report(Order &order, const BIST::ITCHOrderExecutedWithPriceMessage &msg) noexcept
{
    fill_itch_exec_report(order, *reinterpret_cast<const BIST::ITCHOrderExecutedMessage *>(&msg));
    order.price = static_cast<int64_t>(msg.trade_price);
}

void FillerITCH_BIST::fill_itch_delete(Order &order, const BIST::ITCHOrderDeleteMessage &msg) noexcept
{
    order.status = Status::Cancelled;
    order.replaced_quantity = -1 * order.quantity;
    order.last_update_time = msg.timestamp_ns;
    order.cancelled_count++;
    order.message_type = msg.message_type;
}

void FillerITCH_BIST::fill_itch_trade(Order &order, const BIST::ITCHTradeMessage &msg, Venue venue) noexcept
{
    if (UNLIKELY(order.isOurOrder))
        return;

    order.order_id = ++bist_trade_id; 
    order.filled_quantity += msg.quantity;
    order.last_exec_quantity = msg.quantity;
    order.last_update_time = msg.timestamp_ns;
    order.remaining_quantity = 0;
    order.instrument_id = msg.order_book_id;

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

    order.price = static_cast<int64_t>(msg.trade_price);
    order.status = Status::Filled;
    if (msg.side == ' ')
        order.side = Side::Unknown;
    else
        order.side = (msg.side == 'B') ? Side::Buy : Side::Sell;

    order.protocol = Protocol::ITCH;
    order.message_type = msg.message_type;
}