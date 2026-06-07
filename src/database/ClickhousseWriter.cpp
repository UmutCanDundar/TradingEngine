#include "ClickhouseWriter.h"
#include "Order.h"

#include <cstring>
#include <variant>



using namespace clickhouse;

ClickhouseWriter::ClickhouseWriter(spscDbQueue_t &store_to_db, spscDbQueue_t &db_to_parser, const std::string &host, int port) noexcept : store_to_db_(store_to_db), db_to_parser_(db_to_parser)
{
   clickhouse::ClientOptions options;
   options.SetHost(host);
   options.SetPort(port);
   // client_ = std::make_unique<clickhouse::Client>(options);

}

bool ClickhouseWriter::store() noexcept
{
   DbData_t data;
   if (!store_to_db_.pop(data))
      return false;

   
   std::visit([this, &data](const auto &item)
   { 
      // this->insert(item); 
      using T = std::decay_t<decltype(item)>;

      if constexpr (!std::is_same_v<T, Order*>)
         db_to_parser_.push(data);
   }, data);

   return true;
}

// === INSERT FIX ===
void ClickhouseWriter::insert(const MessageWithVenue<FIXMessage *> &fixMsg) 
{
   Block block;
   const auto *msg = fixMsg.msg;

   auto col_transact_time = std::make_shared<ColumnUInt64>();
   col_transact_time->Append(msg->transact_time);
   block.AppendColumn("transact_time", col_transact_time);

   auto col_venue = std::make_shared<ColumnString>();
   col_venue->Append(venue_to_string(fixMsg.venue));
   block.AppendColumn("venue", col_venue);

   auto col_protocol = std::make_shared<ColumnString>();
   col_protocol->Append(protocol_to_string(Protocol::FIX));
   block.AppendColumn("protocol", col_protocol);

   auto col_msg_type = std::make_shared<ColumnString>();
   col_msg_type->Append(std::string(1, msg->msg_type));
   block.AppendColumn("msg_type", col_msg_type);

   auto col_symbol = std::make_shared<ColumnString>();
   col_symbol->Append(msg->symbol);
   block.AppendColumn("symbol", col_symbol);

   auto col_order_id = std::make_shared<ColumnString>();
   col_order_id->Append(msg->order_id);
   block.AppendColumn("order_id", col_order_id);

   auto col_cl_ord_id = std::make_shared<ColumnString>();
   col_cl_ord_id->Append(msg->cl_ord_id);
   block.AppendColumn("cl_ord_id", col_cl_ord_id);

   auto col_exec_id = std::make_shared<ColumnString>();
   col_exec_id->Append(msg->exec_id);
   block.AppendColumn("exec_id", col_exec_id);

   auto col_price = std::make_shared<ColumnInt64>();
   col_price->Append(msg->price);
   block.AppendColumn("price", col_price);

   auto col_quantity = std::make_shared<ColumnUInt32>();
   col_quantity->Append(msg->quantity);
   block.AppendColumn("quantity", col_quantity);

   auto col_leaves_qty = std::make_shared<ColumnUInt32>();
   col_leaves_qty->Append(msg->leaves_qty);
   block.AppendColumn("leaves_qty", col_leaves_qty);

   auto col_filled_qty = std::make_shared<ColumnUInt32>();
   col_filled_qty->Append(msg->filled_qty);
   block.AppendColumn("filled_qty", col_filled_qty);

   auto col_side = std::make_shared<ColumnString>();
   col_side->Append(std::string(1, msg->side));
   block.AppendColumn("side", col_side);

   auto col_ord_status = std::make_shared<ColumnString>();
   col_ord_status->Append(std::string(1, msg->ord_status));
   block.AppendColumn("ord_status", col_ord_status);

   auto col_exec_type = std::make_shared<ColumnString>();
   col_exec_type->Append(std::string(1, msg->exec_type));
   block.AppendColumn("exec_type", col_exec_type);

   auto col_ord_type = std::make_shared<ColumnString>();
   col_ord_type->Append(std::string(1, msg->ord_type));
   block.AppendColumn("ord_type", col_ord_type);

   auto col_time_in_force = std::make_shared<ColumnString>();
   col_time_in_force->Append(std::string(1, msg->time_in_force));
   block.AppendColumn("time_in_force", col_time_in_force);

   client_->Insert("FIX_Table", block);
}
void ClickhouseWriter::insert(const MessageWithVenue<FIXSessionMessage *> &) 
{}

// === INSERT ITCH ===
void ClickhouseWriter::insert(const MessageWithVenue<BIST::ITCHMessage> &itchMsg) 
{
   std::visit([this, venue = itchMsg.venue](const auto *msg)
              {
         using MsgType = std::remove_pointer_t<decltype(msg)>;

         Block block;

        /*  auto col_timestamp = std::make_shared<ColumnUInt64>();
         col_timestamp->Append(msg->timestamp);
         block.AppendColumn("timestamp", col_timestamp); */

         auto col_venue = std::make_shared<ColumnString>();
         col_venue->Append(venue_to_string(venue));
         block.AppendColumn("venue", col_venue);

         auto col_protocol = std::make_shared<ColumnString>();
         col_protocol->Append(protocol_to_string(Protocol::ITCH));
         block.AppendColumn("protocol", col_protocol);

         auto col_msg_type = std::make_shared<ColumnString>();
         col_msg_type->Append(std::string(1, msg->message_type));
         block.AppendColumn("msg_type", col_msg_type);

         if constexpr (std::is_same_v<MsgType, BIST::ITCHAddOrderMessage>)
         {
            std::string stock = std::string(msg->stock, 8);
            stock.erase(std::find_if(stock.begin(), stock.end(), [](const char &c)
                                     { return c == ' ' || c == '\0'; }),
                        stock.end());

            auto col_stock = std::make_shared<ColumnString>();
            col_stock->Append(stock);
            block.AppendColumn("stock", col_stock);

            auto col_order_ref = std::make_shared<ColumnUInt64>();
            col_order_ref->Append(msg->order_ref);
            block.AppendColumn("order_ref", col_order_ref);

            auto col_price = std::make_shared<ColumnUInt32>();
            col_price->Append(msg->price);
            block.AppendColumn("price", col_price);

            auto col_quantity = std::make_shared<ColumnUInt32>();
            col_quantity->Append(msg->quantity);
            block.AppendColumn("quantity", col_quantity);

            auto col_side = std::make_shared<ColumnString>();
            col_side->Append(std::string(1, msg->side));
            block.AppendColumn("side", col_side);
         }
        
         else if constexpr (std::is_same_v<MsgType, BIST::ITCHOrderExecutedMessage>)
         {
            auto col_order_ref = std::make_shared<ColumnUInt64>();
            col_order_ref->Append(msg->order_ref);
            block.AppendColumn("order_ref", col_order_ref);

            auto col_executed_quantity = std::make_shared<ColumnUInt32>();
            col_executed_quantity->Append(msg->executed_quantity);
            block.AppendColumn("executed_quantity", col_executed_quantity);
         }
         else if constexpr (std::is_same_v<MsgType, BIST::ITCHOrderExecutedWithPriceMessage>)
         {
            auto col_order_ref = std::make_shared<ColumnUInt64>();
            col_order_ref->Append(msg->order_ref);
            block.AppendColumn("order_ref", col_order_ref);

            auto col_executed_quantity = std::make_shared<ColumnUInt32>();
            col_executed_quantity->Append(msg->executed_quantity);
            block.AppendColumn("executed_quantity", col_executed_quantity);

            auto col_execution_price = std::make_shared<ColumnUInt32>();
            col_execution_price->Append(msg->execution_price);
            block.AppendColumn("executed_price", col_execution_price);
         }
         else if constexpr (std::is_same_v<MsgType, BIST::ITCHOrderDeleteMessage>)
         {
            auto col_order_ref = std::make_shared<ColumnUInt64>();
            col_order_ref->Append(msg->order_ref);
            block.AppendColumn("order_ref", col_order_ref);
         }
         else if constexpr (std::is_same_v<MsgType, BIST::ITCHTradeMessage>)
         {
            std::string stock = std::string(msg->stock, 8);
            stock.erase(std::find_if(stock.begin(), stock.end(), [](const char &c)
                                     { return c == ' ' || c == '\0'; }),
                        stock.end());

            std::string match_id = std::string(msg->match_id, 4);
            match_id.erase(std::find_if(match_id.begin(), match_id.end(), [](const char &c)
                                    { return c == ' ' || c == '\0'; }),
                       match_id.end());

            auto col_order_ref = std::make_shared<ColumnUInt64>();
            col_order_ref->Append(msg->order_ref);
            block.AppendColumn("order_ref", col_order_ref);

            auto col_price = std::make_shared<ColumnUInt32>();
            col_price->Append(msg->price);
            block.AppendColumn("price", col_price);

            auto col_quantity = std::make_shared<ColumnUInt32>();
            col_quantity->Append(msg->quantity);
            block.AppendColumn("quantity", col_quantity);

            auto col_match_id = std::make_shared<ColumnString>();
            col_match_id->Append(match_id);
            block.AppendColumn("match_id", col_match_id);
         }
         else if constexpr (std::is_same_v<MsgType, BIST::ITCHSystemEventMessage>)
         {
            auto col_event_code = std::make_shared<ColumnString>();
            col_event_code->Append(std::string(1,msg->event_code));
            block.AppendColumn("event_code", col_event_code);
         }

         else if constexpr (std::is_same_v<MsgType, BIST::ITCHSecondsMessage>)
         {}
         else if constexpr (std::is_same_v<MsgType, BIST::ITCHOrderBookDirectoryMessage>)
         {
         }
         else if constexpr (std::is_same_v<MsgType, BIST::ITCHCombinationOrderBookLegMessage>)
         {
         }
         else if constexpr (std::is_same_v<MsgType, BIST::ITCHTickSizeTableEntryMessage>)
         {
         }
         else if constexpr (std::is_same_v<MsgType, BIST::ITCHShortSellStatusMessage>)
         {
         }
         else if constexpr (std::is_same_v<MsgType, BIST::ITCHOrderBookStateMessage>)
         {
         }
         else if constexpr (std::is_same_v<MsgType, BIST::ITCHOrderBookFlushMessage>)
         {
         }
         else if constexpr (std::is_same_v<MsgType, BIST::ITCHEquilibriumPriceUpdateMessage>)
         {
         }

         client_->Insert("ITCH_Table", block); }, itchMsg.msg);
}
void ClickhouseWriter::insert(const MessageWithVenue<NASDAQ::ITCHMessage> &itchMsg) 
{
   std::visit([this, venue = itchMsg.venue](const auto *msg)
              {
         using MsgType = std::remove_pointer_t<decltype(msg)>;

         Block block;

         auto col_timestamp = std::make_shared<ColumnUInt64>();
         col_timestamp->Append(msg->timestamp);
         block.AppendColumn("timestamp", col_timestamp);

         auto col_venue = std::make_shared<ColumnString>();
         col_venue->Append(venue_to_string(venue));
         block.AppendColumn("venue", col_venue);

         auto col_protocol = std::make_shared<ColumnString>();
         col_protocol->Append(protocol_to_string(Protocol::ITCH));
         block.AppendColumn("protocol", col_protocol);

         auto col_msg_type = std::make_shared<ColumnString>();
         col_msg_type->Append(std::string(1, msg->message_type));
         block.AppendColumn("msg_type", col_msg_type);

         if constexpr (std::is_same_v<MsgType, NASDAQ::ITCHAddOrderMessage>)
         {
            std::string stock = std::string(msg->stock, 8);
            stock.erase(std::find_if(stock.begin(), stock.end(), [](const char &c)
                                     { return c == ' ' || c == '\0'; }),
                        stock.end());

            auto col_stock = std::make_shared<ColumnString>();
            col_stock->Append(stock);
            block.AppendColumn("stock", col_stock);

            auto col_order_ref = std::make_shared<ColumnUInt64>();
            col_order_ref->Append(msg->order_ref);
            block.AppendColumn("order_ref", col_order_ref);

            auto col_price = std::make_shared<ColumnUInt32>();
            col_price->Append(msg->price);
            block.AppendColumn("price", col_price);

            auto col_quantity = std::make_shared<ColumnUInt32>();
            col_quantity->Append(msg->quantity);
            block.AppendColumn("quantity", col_quantity);

            auto col_side = std::make_shared<ColumnString>();
            col_side->Append(std::string(1, msg->side));
            block.AppendColumn("side", col_side);
         }
         else if constexpr (std::is_same_v<MsgType, NASDAQ::ITCHAddOrderMPIDMessage>)
         {
            std::string stock = std::string(msg->stock, 8);
            stock.erase(std::find_if(stock.begin(), stock.end(), [](const char &c)
                                     { return c == ' ' || c == '\0'; }),
                        stock.end());

            std::string mpid = std::string(msg->mpid, 4);
            mpid.erase(std::find_if(mpid.begin(), mpid.end(), [](const char &c)
                                     { return c == ' ' || c == '\0'; }),
                        mpid.end());

            auto col_stock = std::make_shared<ColumnString>();
            col_stock->Append(stock);
            block.AppendColumn("stock", col_stock);

            auto col_order_ref = std::make_shared<ColumnUInt64>();
            col_order_ref->Append(msg->order_ref);
            block.AppendColumn("order_ref", col_order_ref);

            auto col_price = std::make_shared<ColumnUInt32>();
            col_price->Append(msg->price);
            block.AppendColumn("price", col_price);

            auto col_quantity = std::make_shared<ColumnUInt32>();
            col_quantity->Append(msg->quantity);
            block.AppendColumn("quantity", col_quantity);

            auto col_side = std::make_shared<ColumnString>();
            col_side->Append(std::string(1, msg->side));
            block.AppendColumn("side", col_side);

            auto col_mpid = std::make_shared<ColumnString>();
            col_mpid->Append(mpid);
            block.AppendColumn("mpid", col_mpid);
         }
         else if constexpr (std::is_same_v<MsgType, NASDAQ::ITCHCancelMessage>)
         {
            auto col_order_ref = std::make_shared<ColumnUInt64>();
            col_order_ref->Append(msg->order_ref);
            block.AppendColumn("order_ref", col_order_ref);

            auto col_remaining_quantity = std::make_shared<ColumnUInt32>();
            col_remaining_quantity->Append(msg->remaining_quantity);
            block.AppendColumn("remaining_quantity", col_remaining_quantity);
         }
         else if constexpr (std::is_same_v<MsgType, NASDAQ::ITCHExecutedMessage>)
         {
            auto col_order_ref = std::make_shared<ColumnUInt64>();
            col_order_ref->Append(msg->order_ref);
            block.AppendColumn("order_ref", col_order_ref);

            auto col_executed_quantity = std::make_shared<ColumnUInt32>();
            col_executed_quantity->Append(msg->executed_quantity);
            block.AppendColumn("executed_quantity", col_executed_quantity);
         }
         else if constexpr (std::is_same_v<MsgType, NASDAQ::ITCHExecutedWithPriceMessage>)
         {
            auto col_order_ref = std::make_shared<ColumnUInt64>();
            col_order_ref->Append(msg->order_ref);
            block.AppendColumn("order_ref", col_order_ref);

            auto col_executed_quantity = std::make_shared<ColumnUInt32>();
            col_executed_quantity->Append(msg->executed_quantity);
            block.AppendColumn("executed_quantity", col_executed_quantity);

            auto col_execution_price = std::make_shared<ColumnUInt32>();
            col_execution_price->Append(msg->execution_price);
            block.AppendColumn("executed_price", col_execution_price);
         }
         else if constexpr (std::is_same_v<MsgType, NASDAQ::ITCHDeleteMessage>)
         {
            auto col_order_ref = std::make_shared<ColumnUInt64>();
            col_order_ref->Append(msg->order_ref);
            block.AppendColumn("order_ref", col_order_ref);
         }
         else if constexpr (std::is_same_v<MsgType, NASDAQ::ITCHTradeMessage>)
         {
            std::string stock = std::string(msg->stock, 8);
            stock.erase(std::find_if(stock.begin(), stock.end(), [](const char &c)
                                     { return c == ' ' || c == '\0'; }),
                        stock.end());

            std::string match_id = std::string(msg->match_id, 4);
            match_id.erase(std::find_if(match_id.begin(), match_id.end(), [](const char &c)
                                    { return c == ' ' || c == '\0'; }),
                       match_id.end());

            auto col_order_ref = std::make_shared<ColumnUInt64>();
            col_order_ref->Append(msg->order_ref);
            block.AppendColumn("order_ref", col_order_ref);

            auto col_price = std::make_shared<ColumnUInt32>();
            col_price->Append(msg->price);
            block.AppendColumn("price", col_price);

            auto col_quantity = std::make_shared<ColumnUInt32>();
            col_quantity->Append(msg->quantity);
            block.AppendColumn("quantity", col_quantity);

            auto col_match_id = std::make_shared<ColumnString>();
            col_match_id->Append(match_id);
            block.AppendColumn("match_id", col_match_id);
         }
         else if constexpr (std::is_same_v<MsgType, NASDAQ::ITCHSystemEventMessage>)
         {
            auto col_event_code = std::make_shared<ColumnString>();
            col_event_code->Append(std::string(1,msg->event_code));
            block.AppendColumn("event_code", col_event_code);
         }
         else if constexpr (std::is_same_v<MsgType, NASDAQ::ITCHReplaceMessage>)
         {}
         else if constexpr (std::is_same_v<MsgType, NASDAQ::ITCHStockDirectoryMessage>)
         {
         }
         else if constexpr (std::is_same_v<MsgType, NASDAQ::ITCHTradingStateMessage>)
         {
         }

            client_->Insert("ITCH_Table", block); }, itchMsg.msg);
}

// === INSERT OUCH ===
void ClickhouseWriter::insert(const MessageWithVenue<BIST::OUCHMessage> &ouchMsg) 
{
   std::visit([/*this, venue = ouchMsg.venue*/](const auto *msg)
              {
      using MsgType = std::remove_pointer_t<decltype(msg)>;

      if constexpr (std::is_same_v<MsgType, BIST::RX::OUCHOrderAcceptedMessage>)
      {
      }
   else if constexpr (std::is_same_v<MsgType, BIST::RX::OUCHOrderRejectedMessage>)
   {
   }
   else if constexpr (std::is_same_v<MsgType, BIST::RX::OUCHOrderReplacedMessage>)
   {
   }
   else if constexpr (std::is_same_v<MsgType, BIST::RX::OUCHOrderCancelledMessage>)
   {
   }
   else if constexpr (std::is_same_v<MsgType, BIST::RX::OUCHOrderExecutedMessage>)
   {
   } }, ouchMsg.msg);
}
void ClickhouseWriter::insert(const MessageWithVenue<NASDAQ::OUCHMessage> &ouchMsg) 
{
   std::visit([/*this, venue = ouchMsg.venue*/](const auto *msg)
              {
                    using MsgType = std::remove_pointer_t<decltype(msg)>;

                    if constexpr (std::is_same_v<MsgType, NASDAQ::RX::OUCHSystemEventMessage>)
                    {
                    }
                    else if constexpr (std::is_same_v<MsgType, NASDAQ::RX::OUCHOrderAcceptedMessage>)
                    {
                    }
                    else if constexpr (std::is_same_v<MsgType, NASDAQ::RX::OUCHOrderReplacedMessage>)
                    {
                    }
                    else if constexpr (std::is_same_v<MsgType, NASDAQ::RX::OUCHOrderCancelledMessage>)
                    {
                    }
                    else if constexpr (std::is_same_v<MsgType, NASDAQ::RX::OUCHOrderExecutedMessage>)
                    {
                    }
                    else if constexpr (std::is_same_v<MsgType, NASDAQ::RX::OUCHOrderRejectedMessage>)
                    {
                    }
                    else if constexpr (std::is_same_v<MsgType, NASDAQ::RX::OUCHBrokenTradeMessage>)
                    {
                    }
                    else if constexpr (std::is_same_v<MsgType, NASDAQ::RX::OUCHOrderModifiedMessage>)
                    {
                    }
                    else if constexpr (std::is_same_v<MsgType, NASDAQ::RX::OUCHCancelPendingMessage>)
                    {
                    }
                    else if constexpr (std::is_same_v<MsgType, NASDAQ::RX::OUCHCancelRejectMessage>)
                    {
                    }
                    else if constexpr (std::is_same_v<MsgType, NASDAQ::RX::OUCHAccountQueryResponse>)
                    {
                    } },
              ouchMsg.msg);
}

// === INSERT ORDER ===
void ClickhouseWriter::insert(const Order *order) 
{
   std::string symbol = std::string(order->symbol.data(), 8);
   symbol.erase(std::find_if(symbol.begin(), symbol.end(), [](const char &c)
                             { return c == ' ' || c == '\0'; }),
                symbol.end());

   Block block;

   auto col_timestamp = std::make_shared<ColumnUInt64>();
   col_timestamp->Append(order->timestamp);
   block.AppendColumn("timestamp", col_timestamp);

   auto col_last_update_time = std::make_shared<ColumnUInt64>();
   col_last_update_time->Append(order->last_update_time);
   block.AppendColumn("last_update_time", col_last_update_time);

   auto col_venue = std::make_shared<ColumnString>();
   col_venue->Append(venue_to_string(order->venue));
   block.AppendColumn("venue", col_venue);

   auto col_protocol = std::make_shared<ColumnString>();
   col_protocol->Append(protocol_to_string(order->protocol));
   block.AppendColumn("protocol", col_protocol);

   auto col_order_id = std::make_shared<ColumnUInt64>();
   col_order_id->Append(order->order_id);
   block.AppendColumn("order_id", col_order_id);

   auto col_client_order_id = std::make_shared<ColumnUInt64>();
   col_client_order_id->Append(order->client_order_id);
   block.AppendColumn("client_order_id", col_client_order_id);

   auto col_price = std::make_shared<ColumnInt64>();
   col_price->Append(order->price);
   block.AppendColumn("price", col_price);

   auto col_quantity = std::make_shared<ColumnUInt32>();
   col_quantity->Append(order->quantity);
   block.AppendColumn("quantity", col_quantity);

   auto col_filled_quantity = std::make_shared<ColumnUInt32>();
   col_filled_quantity->Append(order->filled_quantity);
   block.AppendColumn("filled_quantity", col_filled_quantity);

   auto col_remaining_quantity = std::make_shared<ColumnUInt32>();
   col_remaining_quantity->Append(order->remaining_quantity);
   block.AppendColumn("remaining_quantity", col_remaining_quantity);

   auto col_symbol = std::make_shared<ColumnString>();
   col_symbol->Append(symbol);
   block.AppendColumn("symbol", col_symbol);

   auto col_instrument_id = std::make_shared<ColumnUInt32>();
   col_instrument_id->Append(order->instrument_id);
   block.AppendColumn("instrument_id", col_instrument_id);

   auto col_side = std::make_shared<ColumnUInt8>();
   col_side->Append(static_cast<uint8_t>(order->side));
   block.AppendColumn("side", col_side);

   auto col_status = std::make_shared<ColumnString>();
   col_status->Append(status_to_string(order->status));
   block.AppendColumn("status", col_status);

   auto col_message_type = std::make_shared<ColumnUInt16>();
   col_message_type->Append(order->message_type);
   block.AppendColumn("message_type", col_message_type);

   auto col_time_in_force = std::make_shared<ColumnUInt8>();
   col_time_in_force->Append(static_cast<uint8_t>(order->time_in_force));
   block.AppendColumn("time_in_force", col_time_in_force);

   auto col_order_type = std::make_shared<ColumnUInt8>();
   col_order_type->Append(static_cast<uint8_t>(order->order_type));
   block.AppendColumn("order_type", col_order_type);

   client_->Insert("OrdersTable", block);
}

std::string_view ClickhouseWriter::venue_to_string(Venue venue) noexcept
{
   switch (venue)
   {
   case Venue::NASDAQ:
      return "NASDAQ";
   case Venue::BIST:
      return "BIST";
   default:
      __builtin_unreachable();
   }
   }

   std::string_view ClickhouseWriter::protocol_to_string(Protocol protocol) noexcept
   {
      switch (protocol)
      {
      case Protocol::FIX:
         return "FIX";
      case Protocol::ITCH:
         return "ITCH";
      case Protocol::OUCH:
         return "OUCH";
      default:
         __builtin_unreachable();
      }
   }
   std::string_view ClickhouseWriter::status_to_string(Status status) noexcept
   {
      switch (status)
      {
      case Status::Unknown:
         return "Unknown";
      case Status::New:
         return "New";
      case Status::Partial:
         return "Partial";
      case Status::Filled:
         return "Filled";
      case Status::Cancelled:
         return "Cancelled";
      case Status::Replaced:
         return "Replaced";
      case Status::Expired:
         return "Expired";
      default:
         __builtin_unreachable();
      }
   }