#include "Store_RAM.h"

Store_RAM::Store_RAM() noexcept
{
   order_pool_.resize(ORDER_POOL_SIZE);
   order_map_.reserve(ORDER_POOL_SIZE);
}

void Store_RAM::handle_instrument_definition(const SBEInstrumentDefinitionMessage &msg) noexcept
{
   SymbolMeta meta;
   std::string_view symbol_src(reinterpret_cast<const char *>(msg.currencyCode), 3);
   copy_symbol(meta.symbol, symbol_src);
   meta.lot_size = msg.lotSize;
   meta.tick_size = 1; // Eğer feed veriyorsa gerçek tick_size kullanılır

   instrument_cache_[msg.instrumentId] = meta;
}

void Store_RAM::dispatch_update_order(const MessageWithVenue<std::variant<FIXMessage, ITCHMessage, SBEMessage>> &msgWithVenue) noexcept
{
   std::visit([this, &msgWithVenue](const auto &inner_msg)
              {
            using MsgType = std::decay_t<decltype(inner_msg)>;
            this->dispatch_update_order(MessageWithVenue<MsgType>{inner_msg, msgWithVenue.venue}); }, msgWithVenue.msg);
}

Order *Store_RAM::add_order(uint64_t order_id, Protocol protocol, Venue venue) noexcept
{
   if (UNLIKELY(next_slot >= ORDER_POOL_SIZE))
      return nullptr;

   Order *order = &order_pool_[next_slot++];
   order_map_[OrderKey{order_id, protocol, venue}] = order;
   return order;
}

Order *Store_RAM::get_order(uint64_t order_id, Protocol protocol, Venue venue) noexcept
{
   auto it = order_map_.find(OrderKey{order_id, protocol, venue});
   return (it != order_map_.end()) ? it->second : nullptr;
}

void Store_RAM::update_order(const MessageWithVenue<FIXMessage> &fixMsg) noexcept
{
   const auto &msg = fixMsg.msg;

   alignas(64) constexpr auto allowed_exec_type = []
   {
      std::array<uint8_t, 128> arr = {};
      arr['0'] = 1;
      arr['1'] = 1;
      arr['2'] = 1;
      arr['4'] = 1;
      arr['8'] = 1;
      arr['6'] = 1;
      arr['5'] = 1;
      arr['E'] = 1; // 69
      arr['C'] = 1;
      return arr;
   }();

   uint64_t order_id = absl::Hash<std::string_view>{}(msg.cl_ord_id);
   Order *order = get_order(order_id, Protocol::FIX, fixMsg.venue);
   if (!order)
   {
      order = add_order(order_id, Protocol::FIX, fixMsg.venue);
      if (UNLIKELY(!order))
         return;
   }

   switch (msg.msg_type)
   {
   case 'D': // NewOrderSingle
      fill_fix_new(*order, msg);
      break;
   case 'F': // CancelRequest
      fill_fix_cancel(*order, msg);
      break;
   case 'G': // ModifyRequest
      fill_fix_modify(*order, msg);
      break;
   case '8': // ExecutionReport
      if (allowed_exec_type[static_cast<uint8_t>(msg.exec_type)])
         fill_fix_exec_report(*order, msg);
      else
         return;
      break;
   case '9': // OrderCancelReject
      fill_fix_cancel_reject(*order, msg);
      break;
   }

   update_queue.push(order);
}

// ================= ITCH Handler ===================
void Store_RAM::update_order(const MessageWithVenue<ITCHMessage> &itchMsg) noexcept
{
   std::visit([this, venue = itchMsg.venue](const auto &msg)
              {
            if constexpr (requires { msg.order_ref; }) 
            {
                uint64_t order_id = msg.order_ref;
                Order *order = this->get_order(order_id, Protocol::ITCH, venue);
                if (!order)
                {
                    order = this->add_order(order_id, Protocol::ITCH, venue);
                    if (UNLIKELY(!order))
                        return;
                }

                using MsgType = std::decay_t<decltype(msg)>;

                if constexpr (std::is_same_v<MsgType, ITCHAddOrderMessage> ||
                            std::is_same_v<MsgType, ITCHAddOrderMPIDMessage>)
                {
                    this->fill_itch_add(*order, msg);
                }
                else if constexpr (std::is_same_v<MsgType, ITCHCancelMessage>)
                {
                    this->fill_itch_cancel(*order, msg);
                }
                else if constexpr (std::is_same_v<MsgType, ITCHExecutedMessage> ||
                                std::is_same_v<MsgType, ITCHExecutedWithPriceMessage>)
                {
                    this->fill_itch_exec_report(*order, msg);
                }
                else if constexpr (std::is_same_v<MsgType, ITCHDeleteMessage>)
                {
                    this->fill_itch_delete(*order, msg);
                }
                else if constexpr (std::is_same_v<MsgType, ITCHTradeMessage>)
                {
                    this->fill_itch_trade(*order, msg);
                }

                this->update_queue.push(order);
            } }, itchMsg.msg);
}

// ================= SBE Handler ===================
void Store_RAM::update_order(const MessageWithVenue<SBEMessage> &sbeMsg) noexcept
{
   std::visit([this, venue = sbeMsg.venue](const auto &msg)
              {

            using MsgType = std::decay_t<decltype(msg)>;

            if constexpr (std::is_same_v<MsgType, SBEAddOrderMessage> ||
                            std::is_same_v<MsgType, SBEModifyOrderMessage> ||
                            std::is_same_v<MsgType, SBEDeleteOrderMessage> ||
                            std::is_same_v<MsgType, SBETradeMessage>)
            {
                uint64_t order_id = msg.orderId;
                Order *order = this->get_order(order_id, Protocol::SBE, venue);
                if (!order)
                {
                    order = this->add_order(order_id, Protocol::SBE, venue);
                    if (UNLIKELY(!order))
                        return;
                }

                if constexpr (std::is_same_v<MsgType, SBEAddOrderMessage>)
                {
                    this->fill_sbe_add(*order, msg);
                }
                else if constexpr (std::is_same_v<MsgType, SBEModifyOrderMessage>)
                {
                    this->fill_sbe_modify(*order, msg);
                }
                else if constexpr (std::is_same_v<MsgType, SBEDeleteOrderMessage>)
                {
                    this->fill_sbe_cancel(*order, msg);
                }
                else if constexpr (std::is_same_v<MsgType, SBETradeMessage>)
                {
                    this->fill_sbe_trade(*order, msg);
                }

                this->update_queue.push(order);
            }
            else if constexpr (std::is_same_v<MsgType, SBEInstrumentDefinitionMessage>)
            {
                this->handle_instrument_definition(msg);
            } }, sbeMsg.msg);
}

void Store_RAM::fill_fix_exec_report(Order &order, const FIXMessage &msg) noexcept
{
   switch (msg.ord_status)
   {
   case '0': // New
      order.status = Status::New;
      break;
   case '1': // Partially Filled
      order.status = Status::Partial;
      break;
   case '2': // Filled
      order.status = Status::Filled;
      break;
   case '4': // Cancelled
      if (order.filled_quantity > 0)
         order.status = Status::PartiallyFilled_Cancelled;
      else
         order.status = Status::Cancelled;
      break;
   case '8': // Rejected
      order.status = Status::Rejected;
      break;
   case '6': // Pending Cancel
      order.status = Status::PendingCancel;
      break;
   case '5':                      // Replaced
      order.status = Status::New; // Active again after modification
      break;
   case 'E': // Pending Replace
      order.status = Status::PendingReplace;
      break;
   case 'C': // Expired
      if (order.filled_quantity > 0)
         order.status = Status::PartiallyFilled_Cancelled;
      else
         order.status = Status::Expired;
      break;
   }

   order.filled_quantity = msg.filled_qty;
   order.quantity = msg.leaves_qty + msg.filled_qty;
   order.last_update_time = static_cast<uint64_t>(msg.transact_time);
}