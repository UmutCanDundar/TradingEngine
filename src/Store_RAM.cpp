#include "Store_RAM.h"

Store_RAM::Store_RAM(spscMessageQueue_t &parser_to_store, spscOrderQueue_t &store_to_strategy, spscOrderQueue_t &store_to_strategy_free_slot, spscOrderQueue_t &store_to_risk, spscDbQueue_t &store_to_db, MarketBook &marketbook,HashTables &hashtables) noexcept
    : parser_to_store_(parser_to_store), store_to_strategy_(store_to_strategy), store_to_strategy_free_slot_(store_to_strategy_free_slot), store_to_risk_(store_to_risk), store_to_db_(store_to_db), marketbook_(marketbook), hashtables_(hashtables)
{
   market_order_map_.reserve(ORDER_POOL_CAPACITY);
   our_order_map_.reserve(ORDER_POOL_CAPACITY);

   for (size_t i = MARKETORDER_LAST_INDEX; i < ORDER_POOL_CAPACITY; i++)
      store_to_strategy_free_slot_.push(&order_pool_[i]);

   for (size_t venue = 0; venue < our_orders_all_venue_.size(); venue++) {
      size_t symbol_count = hashtables_.get_symbol_count(venue);
      our_orders_all_venue_[venue].resize(symbol_count);   
   }
}

void Store_RAM::handle_instrument_definition(const SBEInstrumentDefinitionMessage *msg, Venue venue) noexcept
{
   auto [it, inserted] = instrument_cache_[static_cast<std::underlying_type_t<Venue>>(venue)].emplace(msg->instrumentId, SymbolMeta{msg->instrumentId});
   copy_symbol(it->second.symbol, msg->symbol);
}

void Store_RAM::handle_instrument_definition(const ITCHStockDirectoryMessage *msg, Venue venue) noexcept
{
   auto [it, inserted] = instrument_cache_[static_cast<std::underlying_type_t<Venue>>(venue)].emplace(msg->stock_locate, SymbolMeta{msg->stock_locate});
   copy_symbol(it->second.symbol, msg->stock);
}

void Store_RAM::store() noexcept
{
   MessageWithVenue<std::variant<FIXMessage *, ITCHMessage, SBEMessage>> msgWithVenue;

   if (!pending_to_strategy_.empty() && ((pending_to_strategy_.front().order->canModify.load(std::memory_order_relaxed)) == (STRATEGY_DONE | RISK_DONE)))
   {
      PendingMessage<MessageWithVenue<std::variant<FIXMessage *, ITCHMessage, SBEMessage>>> PendingMessage;
      pending_to_strategy_.pop(PendingMessage);
      PendingMessage.order->canModify = 0x00;
      msgWithVenue = PendingMessage.msgWithVenue;
   }
   else
   {
      parser_to_store_.pop(msgWithVenue);
   }

   std::visit([this, &msgWithVenue](const auto &inner_msg)
              {
            using MsgType = std::decay_t<decltype(inner_msg)>;
            this->store(MessageWithVenue<MsgType>{inner_msg, msgWithVenue.venue}); }, msgWithVenue.msg);
}

Order *Store_RAM::add_our_order(Order *order) noexcept
{
   if (UNLIKELY(our_next_slot >= ORDER_POOL_CAPACITY))
   {
      our_next_slot = MARKETORDER_LAST_INDEX;
      our_map_full = true;
   }
   if (our_map_full)
      our_order_map_.erase(OrderKey{order_pool_[our_next_slot].order_id, order_pool_[our_next_slot].protocol, order_pool_[our_next_slot].venue});

   our_order_map_[OrderKey{order->client_order_id, order->protocol, order->venue}] = order;

   //order history per symbol
   auto &orderhistory_for_a_venue = our_orders_all_venue_[static_cast<uint8_t>(order->venue)];
   auto &orderhistory_for_a_symbol = orderhistory_for_a_venue[hashtables_.getIndex(static_cast<uint8_t>(order->venue),order->symbol)];
   auto &orders = orderhistory_for_a_symbol.orders;
   auto &index = orderhistory_for_a_symbol.write_index;
   
   orders[index & (orders.size() - 1)] = order;
   index++;

   return order;
}

Order *Store_RAM::add_market_order(uint64_t order_id, Protocol protocol, Venue venue) noexcept
{

   if (UNLIKELY(market_next_slot >= MARKETORDER_LAST_INDEX))
   {
      market_next_slot = 0;
      market_map_full = true;
   }
   if (market_map_full)
      market_order_map_.erase(OrderKey{order_pool_[market_next_slot].order_id, order_pool_[market_next_slot].protocol, order_pool_[market_next_slot].venue});

   Order *order = &order_pool_[market_next_slot];
   market_order_map_[OrderKey{order_id, protocol, venue}] = order;
   market_next_slot++;
   return order;
}

Order *Store_RAM::get_order(uint64_t order_id, Protocol protocol, Venue venue) noexcept
{
   auto it = our_order_map_.find(OrderKey{order_id, protocol, venue});
   if (it != our_order_map_.end())
   {
      return it->second;
   }

   it = market_order_map_.find(OrderKey{order_id, protocol, venue});
   if (it != market_order_map_.end())
   {
      return it->second;
   }

   return nullptr;
}

void Store_RAM::update_order(const MessageWithVenue<FIXMessage *> &fixMsg) noexcept
{
   const auto *msg = fixMsg.msg;

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

   uint64_t order_id = absl::Hash<std::string_view>{}(msg->cl_ord_id);
   Order *order = get_order(order_id, Protocol::FIX, fixMsg.venue);
   if (!order)
   {
      order = add_market_order(order_id, Protocol::FIX, fixMsg.venue);
      if (UNLIKELY(!order))
         return;
   }
   else if (UNLIKELY(order->canModify == 0x00))
   {
      MessageWithVenue<std::variant<FIXMessage *, ITCHMessage, SBEMessage>> resetMsg(fixMsg.msg, fixMsg.venue);
      pending_to_strategy_.push(PendingMessage<MessageWithVenue<std::variant<FIXMessage *, ITCHMessage, SBEMessage>>>(resetMsg, order));
      return;
   }

   switch (msg->msg_type)
   {
   case '8': // ExecutionReport
      if (allowed_exec_type[static_cast<uint8_t>(msg->exec_type)])
         fill_fix_exec_report(*order, msg, fixMsg.venue);
      else
         return;
      break;
   case '9': // OrderCancelReject
      fill_fix_cancel_reject(*order, msg);
      break;
   }

   store_to_db_.push(fixMsg);
   store_to_db_.push(order);

   if (UNLIKELY(order->isOurOrder && order->status == Status::Filled))
   {
      if (order->canModify == RISK_DONE)
         ReleaseOrder(OrderKey{order->order_id, order->protocol, order->venue});
      else
         store_to_risk_.push(order);
   }
   else
   {
      store_to_strategy_.push(order);
      store_to_risk_.push(order);
   }
}

// ================= ITCH Handler ===================
void Store_RAM::update_order(const MessageWithVenue<ITCHMessage> &itchMsg) noexcept
{
   std::visit([this, &itchMsg](const auto *msg)
              {

               using MsgType = std::remove_pointer_t<decltype(msg)>;

               if constexpr (requires { msg->order_ref; })
               {
                     uint64_t order_id = msg->order_ref;
                     Order *order = this->get_order(order_id, Protocol::ITCH, itchMsg.venue);
                     if (!order)
                     {
                        order = this->add_market_order(order_id, Protocol::ITCH, itchMsg.venue);
                        if (UNLIKELY(!order))
                           return;
                     }
                     else if (UNLIKELY(order->canModify == 0x00))
                     {
                        MessageWithVenue<std::variant<FIXMessage *, ITCHMessage, SBEMessage>> resetMsg(itchMsg.msg, itchMsg.venue);
                        pending_to_strategy_.push(PendingMessage<MessageWithVenue<std::variant<FIXMessage *, ITCHMessage, SBEMessage>>>(resetMsg, order));
                        return;
                     }

                     if constexpr (std::is_same_v<MsgType, ITCHAddOrderMessage> ||
                                 std::is_same_v<MsgType, ITCHAddOrderMPIDMessage>)
                     {
                        this->fill_itch_add(*order, msg, itchMsg.venue);
                        this->marketbook_.add_order(*order);
                     }
                     else if constexpr (std::is_same_v<MsgType, ITCHCancelMessage>)
                     {
                        this->fill_itch_cancel(*order, msg);
                        this->marketbook_.cancel_order(*order);
                     }
                     else if constexpr (std::is_same_v<MsgType, ITCHExecutedMessage> ||
                                       std::is_same_v<MsgType, ITCHExecutedWithPriceMessage>)
                     {
                        this->fill_itch_exec_report(*order, msg);
                     }
                     else if constexpr (std::is_same_v<MsgType, ITCHDeleteMessage>)
                     {
                        this->fill_itch_delete(*order, msg);
                        this->marketbook_.delete_order(*order);
                     }
                     else if constexpr (std::is_same_v<MsgType, ITCHTradeMessage>)
                     {
                        this->fill_itch_trade(*order, msg, itchMsg.venue);
                        this->marketbook_.trade_order(*order);
                     }
                    
                     this->store_to_db_.push(order);

                     if (UNLIKELY(order->isOurOrder && order->status == Status::Filled))
                     {
                        if (order->canModify == RISK_DONE)
                           this->ReleaseOrder(OrderKey{order->order_id, order->protocol, order->venue});
                        else
                           this->store_to_risk_.push(order);
                     }
                     else
                     {
                        this->store_to_strategy_.push(order);
                        this->store_to_risk_.push(order);
                     }
               }
               else if constexpr (std::is_same_v<MsgType, ITCHStockDirectoryMessage>)
               {
                  this->handle_instrument_definition(msg, itchMsg.venue);
               }
               else if constexpr (std::is_same_v<MsgType, ITCHTradingStateMessage>)
               {
                  bool halted = msg->trading_state != 'T';
                  this->update_symbol_halt_status(msg->stock_locate, itchMsg.venue, halted);
               }
               else if constexpr (std::is_same_v<MsgType, ITCHSystemEventMessage>)
               {
                  static constexpr std::array<std::pair<bool,bool>, 128> event_map = [] {
                  std::array<std::pair<bool,bool>, 128> arr{};
                  arr['Q'] = {false, false}; // market start
                  arr['R'] = {false, false}; // resume
                  arr['H'] = {true, false};  // halt
                  arr['M'] = {true, false};  // market close
                  arr['A'] = {true, true};   // circuit breaker
                  return arr;
                  } ();

                  const auto &st = event_map[msg->event_code];
                  this->update_venue_halt_status(itchMsg.venue, st.halted, st.cb);
               }

               this->store_to_db_.push(itchMsg);

            }, itchMsg.msg);
}

// ================= SBE Handler ===================
void Store_RAM::update_order(const MessageWithVenue<SBEMessage> &sbeMsg) noexcept
{
   std::visit([this, &sbeMsg](const auto *msg)
            {
               using MsgType = std::remove_pointer_t<decltype(msg)>;

               if constexpr (std::is_same_v<MsgType, SBEAddOrderMessage> ||
                              std::is_same_v<MsgType, SBEModifyOrderMessage> ||
                              std::is_same_v<MsgType, SBEDeleteOrderMessage> ||
                              std::is_same_v<MsgType, SBETradeMessage>)
               {
                     uint64_t order_id = msg->orderId;
                     Order *order = this->get_order(order_id, Protocol::SBE, sbeMsg.venue);
                     if (!order)
                     {
                        order = this->add_market_order(order_id, Protocol::SBE, sbeMsg.venue);
                        if (UNLIKELY(!order))
                           return;
                     }
                     else if (UNLIKELY(order->canModify == 0x00))
                     {
                        MessageWithVenue<std::variant<FIXMessage *, ITCHMessage, SBEMessage>> resetMsg(sbeMsg.msg, sbeMsg.venue);
                        pending_to_strategy_.push(PendingMessage<MessageWithVenue<std::variant<FIXMessage *, ITCHMessage, SBEMessage>>>(resetMsg, order));
                        return;
                     }

                     if constexpr (std::is_same_v<MsgType, SBEAddOrderMessage>)
                     {
                        this->fill_sbe_add(*order, msg, sbeMsg.venue);
                        this->marketbook_.add_order(*order);
                     }
                     else if constexpr (std::is_same_v<MsgType, SBEModifyOrderMessage>)
                     {
                        this->fill_sbe_modify(*order, msg);
                        this->marketbook_.modify_order(*order, msg->newQuantity);
                     }
                     else if constexpr (std::is_same_v<MsgType, SBEDeleteOrderMessage>)
                     {
                        this->fill_sbe_delete(*order, msg);
                        this->marketbook_.delete_order(*order);
                     }
                     else if constexpr (std::is_same_v<MsgType, SBETradeMessage>)
                     {
                        this->fill_sbe_trade(*order, msg, sbeMsg.venue);
                        this->marketbook_.trade_order(*order);
                     }

                     this->store_to_db_.push(order);

                     if (UNLIKELY(order->isOurOrder && order->status == Status::Filled))
                     {
                        if (order->canModify == RISK_DONE)
                           this->ReleaseOrder(OrderKey{order->order_id, order->protocol, order->venue});
                        else
                           this->store_to_risk_.push(order);
                     }
                     else
                     {
                        this->store_to_strategy_.push(order);
                        this->store_to_risk_.push(order);
                     }
               }

               else if constexpr (std::is_same_v<MsgType, SBEInstrumentDefinitionMessage>)
               {
                  this->handle_instrument_definition(msg, sbeMsg.venue);
               }

               else if constexpr (std::is_same_v<MsgType, SBEMarketStatusMessage>)
               {
               
               static constexpr std::array<std::pair<bool,bool>, 4> sbe_state_map = [] {
                  std::array<std::pair<bool,bool>, 4> arr{};
                  arr[0] = {true, false};   // 0 = market halted
                  arr[1] = {false, false};  // 1 = market open
                  arr[2] = {true, true};    // 2 = circuit breaker
                  arr[3] = {false, false};  // 3 = reserved / future states
                  return arr;
               } ();

               const auto &st = sbe_state_map[msg->marketState];

               this->update_venue_halt_status(sbeMsg.venue, st.first, st.second);
               } 

               this->store_to_db_.push(sbeMsg);

            },sbeMsg.msg);
}

void Store_RAM::fill_fix_exec_report(Order &order, const FIXMessage *msg, Venue venue) noexcept
{
   switch (msg->ord_status)
   {
   case '0': // New
      fill_fix_new(order, msg, venue);
      break;
   case '1': // Partially Filled
      fill_fix_partial(order, msg);
      break;
   case '2': // Filled
      fill_fix_filled(order, msg);
      break;
   case '4': // Cancelled
      fill_fix_cancel(order, msg);
      break;
   case '8': // Rejected
      fill_fix_rejected(order, msg);
      break;
   case '6': // Pending Cancel
      fill_fix_pendingcancel(order, msg);
      break;
   case '5': // Replaced
      fill_fix_new(order, msg, venue);
      break;
   case 'E': // Pending Replace
      fill_fix_pendingreplace(order, msg);
      break;
   case 'C': // Expired
      fill_fix_expired(order, msg);
      break;
   }
}