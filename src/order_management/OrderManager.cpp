#include "OrderManager.h"

#include "HashTables.h"
#include "MarketBook.h"

// ============================
// Constructor & Public Methods
// ============================
OrderManager::OrderManager(spscMessageQueue_t &parser_to_store, spscOrderQueue_t &store_to_strategy, spscOrderQueue_t &store_to_strategy_free_slot, spscOrderQueue_t &store_to_risk, spscDbQueue_t &store_to_db, MarketBook &marketbook,HashTables &hashtables) noexcept
    : parser_to_store_(parser_to_store), store_to_strategy_(store_to_strategy), store_to_strategy_free_slot_(store_to_strategy_free_slot), store_to_risk_(store_to_risk), store_to_db_(store_to_db), marketbook_(marketbook), hashtables_(hashtables)
{
   market_orders_.reserve(ORDER_MAP_CAPACITY);
   our_orders_.reserve(ORDER_MAP_CAPACITY);
   our_orders_wtokenkey_.reserve(ORDER_MAP_CAPACITY);
   awaitingAck_orders_.reserve(PENDING_ORDER_SIZE);
   nq_ouch_refnum_ordkey_.reserve(ORDER_MAP_CAPACITY);
   
   for (size_t i = MARKETORDER_LAST_INDEX; i < ORDER_POOL_CAPACITY;i++)
      store_to_strategy_free_slot_.push(&order_pool_[i]);

   for (size_t venue = 0; venue < VENUE_COUNT; venue++) {
      size_t symbol_count = hashtables_.get_symbol_count(venue);
      our_orders_all_venue_[venue].resize(symbol_count);
      instrument_cache_[venue].reserve(symbol_count);
      if(venue == static_cast<size_t>(Venue::NASDAQ))
         nq_ouch_sym_symid_.reserve(symbol_count);
   }
      
}

bool OrderManager::store() noexcept
{
   MessageWithVenue<MessageTypes_t> msgWithVenue;

   if (!pending_to_strategy_.empty() && ((pending_to_strategy_.front().order->canModify.load(std::memory_order_relaxed)) == (STRATEGY_DONE | RISK_DONE | BUILDER_DONE)))
   {
      PendingMessage<MessageWithVenue<MessageTypes_t>> PendingMessage{};
      pending_to_strategy_.pop(PendingMessage);
      msgWithVenue = PendingMessage.msgWithVenue;
   }
   else
   {
      if (!parser_to_store_.pop(msgWithVenue))
         return false;
   }

   const bool is_fix_session_msg =
       std::visit([this](const auto &inner_msg) -> bool
                  {
          using MsgType = std::decay_t<decltype(inner_msg)>;

          if constexpr (std::is_same_v<MsgType, FIXSessionMessage>)
          {
             store_to_db_.push(inner_msg);
             return true; 
          }
          return false; }, msgWithVenue.msg);

   if (is_fix_session_msg)
      return false;

   std::visit([this, &msgWithVenue](const auto &inner_msg)
   {  
      using MsgType = std::decay_t<decltype(inner_msg)>;
      this->store(MessageWithVenue<MsgType>{inner_msg, msgWithVenue.venue}); 
   }, msgWithVenue.msg);

   return true;
}

void OrderManager::add_awaitingAck_order(Order &order) noexcept
{
   awaitingAck_orders_.emplace(order.client_order_id, &order);
   pending_orders_[static_cast<size_t>(order.venue)][pending_next_slot++ & (PENDING_ORDER_SIZE - 1)].store(&order, std::memory_order_release);

   if (order.venue == Venue::NASDAQ)
   {
      auto it = nq_ouch_refnum_ordkey_.find(order.pre_user_ref_num);
      if (it == nq_ouch_refnum_ordkey_.end())
      {
         nq_ouch_refnum_ordkey_.emplace(order.user_ref_num, order.order_id);
      }
      else
      {
         nq_ouch_refnum_ordkey_.erase(order.pre_user_ref_num);
         nq_ouch_refnum_ordkey_.emplace(order.user_ref_num, order.order_id);
      }
   }
}

// ============================
// Order Management Methods
// ============================
Order* OrderManager::add_our_order(Order *order) noexcept
{
   if (static uint64_t triggered = 0; UNLIKELY(our_orders_.size() >= OUR_ORDER_MAP_THRESHOLD))
   {
      if(!(triggered++ & (ORDER_MAP_CAPACITY -1))) 
         our_next_slot = MARKETORDER_LAST_INDEX;

      Order* order_to_release = &order_pool_[our_next_slot];
      auto& orderhistory_for_a_venue = our_orders_all_venue_[static_cast<size_t>(order_to_release->venue)];
      auto& orderhistory_for_a_symbol = orderhistory_for_a_venue[order_to_release->symbol_index];

      release_order(orderhistory_for_a_symbol, *order_to_release);
   }

   our_orders_[OrderKey{order->order_id, order->instrument_id, static_cast<uint8_t>(order->venue), static_cast<uint8_t>(order->side)}] = order;
   if(LIKELY(order->protocol != Protocol::OUCH))
      our_orders_wtokenkey_[order->client_order_id] = order;

   auto &orderhistory_for_a_venue = our_orders_all_venue_[static_cast<size_t>(order->venue)];
   auto &orderhistory_for_a_symbol = orderhistory_for_a_venue[order->symbol_index];
   orderhistory_for_a_symbol.push(order);

   return order;
}

Order* OrderManager::add_market_order(uint64_t order_id, uint32_t instrument_id, Venue venue, uint8_t side) noexcept
{

   if (UNLIKELY(market_next_slot >= MARKETORDER_LAST_INDEX))
   {
      market_next_slot = 0;
      market_map_full = true;
   }
   if (market_map_full)
      market_orders_.erase(OrderKey{order_pool_[market_next_slot].order_id, order_pool_[market_next_slot].instrument_id, static_cast<uint8_t>(order_pool_[market_next_slot].venue), static_cast<uint8_t>(order_pool_[market_next_slot].side)});

   Order* order = &order_pool_[market_next_slot];
   market_orders_[OrderKey{order_id, instrument_id, static_cast<uint8_t>(venue), side}] = order;
   market_next_slot++;
   return order;
}

Order* OrderManager::get_order_from_our_map(OrderKey order_key) noexcept
{
   auto it = our_orders_.find(order_key);
   if (it != our_orders_.end())
      return it->second;
   
   return nullptr;
}
Order* OrderManager::get_order_from_our_map_wtokenkey(uint64_t client_order_id) noexcept
{
   auto it = our_orders_wtokenkey_.find(client_order_id);
   if (it != our_orders_wtokenkey_.end())
      return it->second;

   return nullptr;
}
Order* OrderManager::get_order_from_market_map(uint64_t order_id, uint32_t instrument_id, Venue venue, uint8_t side) noexcept
{
   auto it = market_orders_.find(OrderKey{order_id, instrument_id, static_cast<uint8_t>(venue), side});
   if (it != market_orders_.end())
      return it->second;

   return nullptr;
}
Order* OrderManager::get_order_from_awaitingAck_orders(uint64_t client_order_id) noexcept 
{
   auto it = awaitingAck_orders_.find(client_order_id); 
   if (it != awaitingAck_orders_.end()) 
      return it->second;

   return nullptr;
}

// ==========================
// Exchange Message Handlers
// ==========================
// ================= BIST FIX ===================
void OrderManager::update_order(const MessageWithVenue<FIXMessage *> &fixMsg) noexcept
{
   const auto *msg = fixMsg.msg;

   constexpr auto allowed_exec_type = []
   {
      std::array<uint8_t, 128> arr = {};
      arr['0'] = 1; // order new(status 0) - msgtype 8
      arr['4'] = 1; // cancel req(4) - mt 8
      arr['5'] = 1; // cancel replace req(0,1,2,4) - mt 8
      arr['C'] = 1; // expired(C) - mt 8
      arr['F'] = 1; // order fill(1,2) - mt 8
      arr['L'] = 1; // trig or act by the sis(0) - mt 8
      return arr;
   }();

   uint64_t client_order_id = absl::Hash<std::string_view>{}(msg->cl_ord_id);
   uint64_t order_id = absl::Hash<std::string_view>{}(msg->order_id);
   
   Order *order = get_order_from_our_map(OrderKey{order_id, msg->instrument_id, static_cast<uint8_t>(Venue::BIST), msg->side});

   if(!order)
   {
      order = get_order_from_awaitingAck_orders(client_order_id);
      
      if (UNLIKELY(!order))
         return;
      
      if (UNLIKELY(msg->ord_status == '8')) // Order Rejected (Do not proceed further)
      {
         store_to_strategy_free_slot_.push(order);
         return;
      }

      order = add_our_order(order);
   }
      
   if (UNLIKELY(order->canModify == 0x00))
   {
      MessageWithVenue<MessageTypes_t> resetMsg(fixMsg.msg, fixMsg.venue);
      pending_to_strategy_.push(PendingMessage<MessageWithVenue<MessageTypes_t>>(resetMsg, order));
      return;
   }

   switch (msg->msg_type)
   {
   case '8': // ExecutionReport
      if (hashtables_.is_duplicate_exec_id(msg->exec_id))
         return;

      if (allowed_exec_type[static_cast<uint8_t>(msg->exec_type)])
         filler_fix_.fill_fix_exec_report(*order, *msg);
      else
         return;
      break;
   case '9': // OrderCancelReject
      filler_fix_.fill_fix_cancel_reject(*order, *msg);
      store_to_db_.push(fixMsg);
      store_to_db_.push(order);
      return;
   }

   order->canModify = 0x00;
   awaitingAck_orders_.erase(client_order_id); // In FIX, ClOrdID is unique for every action (New/Replace/Cancel).

   store_to_db_.push(fixMsg);
   store_to_db_.push(order);
   store_to_risk_.push(order);
   store_to_strategy_.push(order);
   
}

// ================= BIST ITCH  ===================
void OrderManager::update_order(const MessageWithVenue<BIST::ITCHMessage> &itchMsg) noexcept 
{
   std::visit([this, &itchMsg](const auto *msg)
              {
                 using MsgType = std::remove_pointer_t<decltype(msg)>;

                 if constexpr (requires { msg->order_id; } || std::is_same_v<MsgType, BIST::ITCHTradeMessage>)
                 {
                     uint8_t side = msg->side == 'B' ? static_cast<uint8_t>(Side::Buy) : static_cast<uint8_t>(Side::Sell);
                     uint64_t order_id = msg->order_id;
                     Order *order = this->get_order_from_market_map(order_id, msg->order_book_id, Venue::BIST, static_cast<uint8_t>(msg->side));
                     if (!order)
                     {
                        if constexpr (std::is_same_v<MsgType, BIST::ITCHAddOrderMessage>)
                        {             
                           if (awaitingAck_orders_.size() > 0 && is_matched_pending_order(*msg)) // For AddOrder messsage types (ignore our orders)
                              return;

                           order = this->add_market_order(order_id, msg->order_book_id, Venue::BIST, side);
                           if (UNLIKELY(!order))
                              return;
                        }
                        else // For all message types other than AddOrder (ignore our orders)
                        {
                           return;
                        }
                     }
                     else if (UNLIKELY(order->canModify == 0x00))
                     {
                        MessageWithVenue<MessageTypes_t> resetMsg(itchMsg.msg, Venue::BIST);
                        pending_to_strategy_.push(PendingMessage<MessageWithVenue<MessageTypes_t>>(resetMsg, order));
                        return;
                     }

                     if constexpr (std::is_same_v<MsgType, BIST::ITCHAddOrderMessage>)
                     {
                        filler_itch_bist_.fill_itch_add(*order, *msg, Venue::BIST);
                        this->marketbook_.add_order(*order);
                     }
                     else if constexpr (std::is_same_v<MsgType, BIST::ITCHOrderExecutedMessage> ||
                                          std::is_same_v<MsgType, BIST::ITCHOrderExecutedWithPriceMessage>)
                     {
                        filler_itch_bist_.fill_itch_exec_report(*order, *msg);
                        this->marketbook_.exec_order(*order);
                     }
                     else if constexpr (std::is_same_v<MsgType, BIST::ITCHOrderDeleteMessage>)
                     {
                        filler_itch_bist_.fill_itch_delete(*order, *msg);
                        this->marketbook_.delete_order(*order);
                     }
                     else if constexpr (std::is_same_v<MsgType, BIST::ITCHTradeMessage>)
                     {
                        filler_itch_bist_.fill_itch_trade(*order, *msg, Venue::BIST);
                     }

                     order->canModify = 0x00;

                     this->store_to_db_.push(order);
                     this->store_to_risk_.push(order);
                     this->store_to_strategy_.push(order);
                 }
                 else if constexpr (std::is_same_v<MsgType, BIST::ITCHOrderBookDirectoryMessage>)
                 {
                    this->handle_instrument_definition(*msg, Venue::BIST);
                 }
                 else if constexpr (std::is_same_v<MsgType, BIST::ITCHOrderBookStateMessage>)
                 {
                    bool halted = strncmp(msg->state_name, "ACTIVE", 6) != 0;
                    this->update_symbol_halt_status(msg->order_book_id, Venue::BIST, halted);
                 }
                 else if constexpr (std::is_same_v<MsgType, BIST::ITCHSystemEventMessage>)
                 {
                    const uint8_t market_closed = (msg->event_code == 'O') ? 0x00 : 0x01;
                    this->update_venue_halt_status(itchMsg.venue, market_closed, 0x00);
                 }
                 else if constexpr (std::is_same_v<MsgType, BIST::ITCHTickSizeTableEntryMessage>)
                 {
                     this->handle_tick_size_definition(msg->order_book_id, msg->tick_size);
                 }
                 else if constexpr (std::is_same_v<MsgType, BIST::ITCHOrderBookFlushMessage>)
                 {
                    const auto venue_index = static_cast<std::underlying_type_t<Venue>>(Venue::BIST);
                    const auto symbol = instrument_cache_[venue_index].find(msg->order_book_id)->second.symbol;
                    const auto symbol_index = hashtables_.getIndex(venue_index, symbol);
                    this->marketbook_.flush(venue_index, symbol_index);
                    this->handle_flush_status(venue_index, symbol_index);
                 }

                 this->store_to_db_.push(itchMsg);
              },
              itchMsg.msg);
}

// ================= NASDAQ ITCH ===================
void OrderManager::update_order(const MessageWithVenue<NASDAQ::ITCHMessage> &itchMsg) noexcept
{
   std::visit([this, &itchMsg](const auto *msg)
              {

               using MsgType = std::remove_pointer_t<decltype(msg)>;

               if constexpr (requires { msg->order_ref; })
               {
                     uint64_t order_id = msg->order_ref;
                     Order *order = this->get_order_from_market_map(order_id, msg->stock_locate, Venue::NASDAQ, 2);
                     
                     if (!order)
                     {
                        if constexpr (std::is_same_v<MsgType, NASDAQ::ITCHAddOrderMessage> ||
                                      std::is_same_v<MsgType, NASDAQ::ITCHAddOrderMPIDMessage>)
                        {
                           if (awaitingAck_orders_.size() > 0 && is_matched_pending_order(*msg)) // For AddOrder messsage types (ignore our orders)
                              return;

                           order = this->add_market_order(order_id, msg->stock_locate, Venue::NASDAQ, 2);
                           if (UNLIKELY(!order))
                              return;
                        }
                        else // For all message types other than AddOrder (ignore our orders)
                        {
                           return;
                        }
                     }
                     else if (UNLIKELY(order->canModify == 0x00))
                     {
                        MessageWithVenue<MessageTypes_t> resetMsg(itchMsg.msg, Venue::NASDAQ);
                        pending_to_strategy_.push(PendingMessage<MessageWithVenue<MessageTypes_t>>(resetMsg, order));
                        return;
                     }
                     
                     if constexpr (std::is_same_v<MsgType, NASDAQ::ITCHAddOrderMessage> ||
                                      std::is_same_v<MsgType, NASDAQ::ITCHAddOrderMPIDMessage>)
                     {
                        filler_itch_nq_.fill_itch_add(*order, *msg, Venue::NASDAQ);
                        this->marketbook_.add_order(*order);
                     }
                     else if constexpr (std::is_same_v<MsgType, NASDAQ::ITCHCancelMessage>)
                     {
                        filler_itch_nq_.fill_itch_cancel(*order, *msg);
                        this->marketbook_.cancel_order(*order);
                     }
                     else if constexpr (std::is_same_v<MsgType, NASDAQ::ITCHExecutedMessage> ||
                                       std::is_same_v<MsgType, NASDAQ::ITCHExecutedWithPriceMessage>)
                     {
                        filler_itch_nq_.fill_itch_exec_report(*order, *msg);
                        this->marketbook_.exec_order(*order);
                     }
                     else if constexpr (std::is_same_v<MsgType, NASDAQ::ITCHDeleteMessage>)
                     {
                        filler_itch_nq_.fill_itch_delete(*order, *msg);
                        this->marketbook_.delete_order(*order);
                     }
                     else if constexpr (std::is_same_v<MsgType, NASDAQ::ITCHReplaceMessage>)
                     {
                        this->marketbook_.delete_order(*order);
                        filler_itch_nq_.fill_itch_replace(*order, *msg, Venue::NASDAQ);
                        this->marketbook_.add_order(*order);
                     }
                     else if constexpr (std::is_same_v<MsgType, NASDAQ::ITCHTradeMessage>)
                     {
                        filler_itch_nq_.fill_itch_trade(*order, *msg, Venue::NASDAQ);
                     }

                     order->canModify = 0x00;

                     this->store_to_db_.push(order);
                     this->store_to_risk_.push(order);
                     this->store_to_strategy_.push(order);
                     
               }
               else if constexpr (std::is_same_v<MsgType, NASDAQ::ITCHStockDirectoryMessage>)
               {
                  this->handle_instrument_definition(*msg, Venue::NASDAQ);
               }
               else if constexpr (std::is_same_v<MsgType, NASDAQ::ITCHTradingStateMessage>)
               {
                  bool halted = msg->trading_state != 'T';
                  this->update_symbol_halt_status(msg->stock_locate, Venue::NASDAQ, halted);
               }
               else if constexpr (std::is_same_v<MsgType, NASDAQ::ITCHSystemEventMessage>)
               {
                  static constexpr std::array<std::pair<bool,bool>, 128> event_map = [] {
                  std::array<std::pair<uint8_t,uint8_t>, 128> arr{};
                  arr['O'] = {0x01, 0x00}; // first message
                  arr['S'] = {0x00, 0x00}; // system start
                  arr['Q'] = {0x00, 0x00}; // market open
                  arr['M'] = {0x01, 0x00}; // market close
                  arr['E'] = {0x01, 0x00}; // system stop (only delete message)
                  arr['C'] = {0x01, 0x00}; // last message
                  return arr;
                  } ();

                  const auto &st = event_map[msg->event_code];
                  this->update_venue_halt_status(Venue::NASDAQ, st.first, st.second);
               }

               this->store_to_db_.push(itchMsg);

            }, itchMsg.msg);
}

// ================= BIST OUCH ===================
void OrderManager::update_order(const MessageWithVenue<BIST::OUCHMessage> &ouchMsg) noexcept
{
   std::visit([this, &ouchMsg](const auto *msg)
              {
                 uint64_t client_order_id = absl::Hash<std::string_view>{}(std::string_view{msg->order_token, 14});
                 using MsgType = std::remove_pointer_t<decltype(msg)>;

                 if constexpr (requires { msg->order_id; })
                 {
                    
                     uint64_t order_id = msg->order_id;
                     Order *order = this->get_order_from_our_map(OrderKey{order_id, msg->order_book_id, static_cast<uint8_t>(Venue::BIST), 2});

                     if (!order)
                     {
                        order = get_order_from_awaitingAck_orders(client_order_id);
                        if (UNLIKELY(!order))
                           return;
                        order = this->add_our_order(order);
                        awaitingAck_orders_.erase(client_order_id);
                     }
                     else if (UNLIKELY(order->canModify == 0x00))
                     {
                        MessageWithVenue<MessageTypes_t> resetMsg(ouchMsg.msg, Venue::BIST);
                        pending_to_strategy_.push(PendingMessage<MessageWithVenue<MessageTypes_t>>(resetMsg, order));
                        return;
                     }

                     if constexpr (std::is_same_v<MsgType, BIST::OUT::OUCHOrderAcceptedMessage>)
                     {
                        filler_ouch_bist_.fill_ouch_accepted(*order, *msg);
                     }
                     else if constexpr (std::is_same_v<MsgType, BIST::OUT::OUCHOrderReplacedMessage>)
                                          
                     {
                        filler_ouch_bist_.fill_ouch_replaced(*order, *msg);
                     }
                     else if constexpr (std::is_same_v<MsgType, BIST::OUT::OUCHOrderCancelledMessage>)
                     {
                        filler_ouch_bist_.fill_ouch_cancelled(*order, *msg);
                     }

                     order->canModify = 0x00;

                     this->store_to_db_.push(order);
                     this->store_to_risk_.push(order);
                     this->store_to_strategy_.push(order);
                 }
                 else 
                 {
                    Order *order = this->get_order_from_our_map_wtokenkey(client_order_id);
                    if (!order)
                    {
                     order = get_order_from_awaitingAck_orders(client_order_id);
                     if (UNLIKELY(!order))
                       return;
                     awaitingAck_orders_.erase(order->client_order_id);
                    }
                    else if (UNLIKELY(order->canModify == 0x00))
                    {
                       MessageWithVenue<MessageTypes_t> resetMsg(ouchMsg.msg, Venue::BIST);
                       pending_to_strategy_.push(PendingMessage<MessageWithVenue<MessageTypes_t>>(resetMsg, order));
                       return;
                    }

                    if constexpr (std::is_same_v<MsgType, BIST::OUT::OUCHOrderExecutedMessage>)
                    {
                       filler_ouch_bist_.fill_ouch_executed(*order, *msg);

                       order->canModify = 0x00;

                       this->store_to_risk_.push(order);
                       this->store_to_strategy_.push(order);
                       this->store_to_db_.push(order);
                    }
                    else if constexpr (std::is_same_v<MsgType, BIST::OUT::OUCHOrderRejectedMessage>)
                    {
                       filler_ouch_bist_.fill_ouch_rejected(*order, *msg);
                    }
                 }

                 this->store_to_db_.push(ouchMsg); 
               }, ouchMsg.msg);
}

// ================= NASDAQ OUCH ===================
void OrderManager::update_order(const MessageWithVenue<NASDAQ::OUCHMessage> &ouchMsg) noexcept
{
   std::visit([this, &ouchMsg](const auto *msg)
              {
                
                 using MsgType = std::remove_pointer_t<decltype(msg)>;

                 if constexpr (requires { msg->order_reference_number; })
                 {
                     uint64_t client_order_id = absl::Hash<std::string_view>{}(std::string_view{msg->cl_ord_id, 14});
                     uint64_t symbol_pack;
                     std::memcpy(&symbol_pack, msg->symbol, 8);
                     uint32_t stock_locate = nq_ouch_sym_symid_.find(symbol_pack)->second;

                     uint64_t order_id = msg->order_reference_number;
                     
                     Order *order = this->get_order_from_our_map(OrderKey{order_id, stock_locate, static_cast<uint8_t>(Venue::NASDAQ), 2});

                     if (!order)
                     {
                        order = get_order_from_awaitingAck_orders(client_order_id);
                        if (UNLIKELY(!order))
                           return;
                        order = this->add_our_order(order);
                        nq_ouch_refnum_ordkey_.emplace(msg->user_ref_num, OrderKey{order_id, stock_locate, static_cast<uint8_t>(Venue::NASDAQ), 2});
                        awaitingAck_orders_.erase(client_order_id);
                     }
                     else if (UNLIKELY(order->canModify == 0x00))
                     {
                        MessageWithVenue<MessageTypes_t> resetMsg(ouchMsg.msg, Venue::NASDAQ);
                        pending_to_strategy_.push(PendingMessage<MessageWithVenue<MessageTypes_t>>(resetMsg, order));
                        return;
                     }

                     if constexpr (std::is_same_v<MsgType, NASDAQ::OUT::OUCHOrderAcceptedMessage>)
                     {
                        filler_ouch_nq_.fill_ouch_accepted(*order, *msg);
                     }
                     else if constexpr (std::is_same_v<MsgType, NASDAQ::OUT::OUCHOrderReplacedMessage>)                 
                     {
                        filler_ouch_nq_.fill_ouch_replaced(*order, *msg);
                     }
                   

                     order->canModify = 0x00;

                     this->store_to_db_.push(order);
                     this->store_to_risk_.push(order);
                     this->store_to_strategy_.push(order);
                 }
                 else if constexpr (requires { msg->user_ref_num; })
                 {

                    if constexpr (std::is_same_v<MsgType, NASDAQ::OUT::OUCHOrderRejectedMessage>) 
                    {
                        uint64_t client_order_id = absl::Hash<std::string_view>{}(std::string_view{msg->cl_ord_id, 14});
                        Order* order = get_order_from_awaitingAck_orders(client_order_id);
                        if (UNLIKELY(!order))
                           return;
                        awaitingAck_orders_.erase(order->client_order_id);
                        
                        filler_ouch_nq_.fill_ouch_rejected(*order, *msg);
                    }
                    else 
                    {
                        OrderKey orderkey = nq_ouch_refnum_ordkey_.find(msg->user_ref_num)->second;
                        Order *order = this->get_order_from_our_map(orderkey);
                        awaitingAck_orders_.erase(order->client_order_id);

                        if (UNLIKELY(order->canModify == 0x00))
                        {
                           MessageWithVenue<MessageTypes_t> resetMsg(ouchMsg.msg, Venue::NASDAQ);
                           pending_to_strategy_.push(PendingMessage<MessageWithVenue<MessageTypes_t>>(resetMsg, order));
                           return;
                        }

                        if constexpr (std::is_same_v<MsgType, NASDAQ::OUT::OUCHOrderCancelledMessage>) 
                        {
                           filler_ouch_nq_.fill_ouch_cancelled(*order, *msg);
                        }
                        else if constexpr (std::is_same_v<MsgType, NASDAQ::OUT::OUCHOrderExecutedMessage>)
                        {
                           filler_ouch_nq_.fill_ouch_executed(*order, *msg);
                        }
                        else if constexpr (std::is_same_v<MsgType, NASDAQ::OUT::OUCHOrderModifiedMessage>)
                        {
                           filler_ouch_nq_.fill_ouch_modified(*order, *msg);
                        }
                        else
                        {
                           this->store_to_db_.push(ouchMsg);
                           return;
                        }

                        order->canModify = 0x00;

                        this->store_to_db_.push(order);
                        this->store_to_risk_.push(order);
                        this->store_to_strategy_.push(order);
                    }
                       
                  }

                  this->store_to_db_.push(ouchMsg);
                  
                  }, ouchMsg.msg);
}

// ==================
// Metadata Handlers
// ==================
void OrderManager::handle_instrument_definition(const BIST::ITCHOrderBookDirectoryMessage &msg, Venue venue) noexcept
{
   auto [it, inserted] = instrument_cache_[static_cast<size_t>(venue)].emplace(msg.order_book_id, std::make_unique<SymbolMeta>(msg.order_book_id, msg.round_lot_size));
   copy_symbol(it->second->symbol, msg.symbol);
}
void OrderManager::handle_instrument_definition(const NASDAQ::ITCHStockDirectoryMessage &msg, Venue venue) noexcept
{
   uint64_t symbol_pack;
   std::memcpy(&symbol_pack, msg.stock, 8);
   nq_ouch_sym_symid_.emplace(symbol_pack, msg.stock_locate);
   auto [it, inserted] = instrument_cache_[static_cast<size_t>(venue)].emplace(msg.stock_locate, std::make_unique<SymbolMeta>(msg.stock_locate, msg.round_lot_size));
   copy_symbol(it->second->symbol, msg.stock);

   static constexpr std::array<std::pair<std::pair<int64_t, int64_t>, uint64_t>, 5> nasdaq_tick_size_ranges = 
   {{{{0, 199999}, 1},
   {{200000, 999999}, 5},
   {{1000000, 4999999}, 10}, // THESE VALUES WILL BE PROVIDED BY NASDAQ
   {{5000000, 9999999}, 50},
   {{10000000, INT64_MAX}, 100}}};

   for (const auto &[price_range, tick_size] : nasdaq_tick_size_ranges)
      handle_tick_size_definition(it->second->tick_size_table, price_range.first, price_range.second, tick_size);
}

void OrderManager::handle_tick_size_definition(const BIST::ITCHTickSizeTableEntryMessage &msg, Venue venue) noexcept
{
   auto symbolmeta_it = instrument_cache_[static_cast<std::underlying_type_t<Venue>>(venue)].find(msg.order_book_id);

   SymbolMeta &symbolmeta = *(symbolmeta_it->second);

   TickSizeEntry *new_entry = &tick_size_entry_pool_[tick_size_bist_next_slot++ & (TICKSIZE_CAPACITY_FOR_BIST - 1)];
   new_entry->price_from = msg.price_from;
   new_entry->price_to = msg.price_to;
   new_entry->tick_size = msg.tick_size;

   for (auto &entry : symbolmeta.tick_size_table)
   {
      TickSizeEntry *existing = entry.load(std::memory_order_relaxed);

      if (!existing)
      {
         entry.store(new_entry, std::memory_order_release);
         return;
      }

      if (existing->price_from == msg.price_from && existing->price_to == msg.price_to)
      {
         existing->price_from = msg.price_from;
         existing->price_to = msg.price_to;
         existing->tick_size = msg.tick_size;
         return;
      }
   }
   symbolmeta.tick_size_table[0].store(new_entry, std::memory_order_release);
}
void OrderManager::handle_tick_size_definition(auto &tick_size_table, int64_t price_from, int64_t price_to, uint64_t tick_size) noexcept
{
   TickSizeEntry *new_entry = &tick_size_entry_pool_[tick_size_nasdaq_next_slot++];
   new_entry->price_from = price_from;
   new_entry->price_to = price_to;
   new_entry->tick_size = tick_size;

   for (auto &entry : tick_size_table)
   {
      TickSizeEntry *existing = entry.load(std::memory_order_relaxed);

      if (!existing)
      {
         entry.store(new_entry, std::memory_order_release);
         return;
      }
   }
}

// ======================
// System Event Handlers
// ======================
void OrderManager::handle_flush_status(const uint8_t venue_index, const uint32_t symbol_index) noexcept
{

   for (auto &order : our_orders_all_venue_[venue_index][symbol_index].orders)
   {
      if (!order)
         continue;

      if (order->status == Status::New)
      {
         order->status = Status::Cancelled;
         order->cancelled_count++;
         order->remaining_quantity = order->quantity;
      }
      else if (order->status == Status::Partial)
      {
         order->status = Status::Cancelled;
         order->cancelled_count++;
         order->remaining_quantity = order->quantity - order->filled_quantity;
      }
      else
      {
         continue;
      }

      store_to_risk_.push(order);
      store_to_strategy_.push(order);
   }
}

void OrderManager::update_venue_halt_status(Venue venue, uint8_t halted, uint8_t circuit_breaker) noexcept
{
   auto venue_index = static_cast<size_t>(venue);
   auto &flags = venue_flags_[venue_index].flags;

   flags.fetch_or(halted, std::memory_order_release);
   flags.fetch_or(circuit_breaker, std::memory_order_release);
}

void OrderManager::update_symbol_halt_status(uint64_t instrument_id, Venue venue, bool halted) noexcept
{
   instrument_cache_[static_cast<size_t>(venue)][instrument_id]->halted.store(halted, std::memory_order_release);
}

void OrderManager::update_symbol_flush_status(uint64_t instrument_id, Venue venue, bool flushed) noexcept
{
   instrument_cache_[static_cast<size_t>(venue)][instrument_id]->flushed.store(flushed, std::memory_order_release);
}