#include "Store_DB.h"

#include <cstring>
#include <variant>

using namespace clickhouse;

Store_DB::Store_DB(spscDbQueue_t &store_to_db, const std::string &host, const int &port) : store_to_db_(store_to_db)
{
   clickhouse::ClientOptions options;
   options.SetHost(host);
   options.SetPort(port);
   client_ = std::make_unique<clickhouse::Client>(options);
}

void Store_DB::insert(const MessageWithVenue<FIXMessage *> &fixMsg)
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

// === INSERT ITCH ===
/* void Store_DB::insert(const MessageWithVenue<BIST::ITCHMessage> &itchMsg)
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

         if constexpr (std::is_same_v<MsgType, ITCHAddOrderMessage> ||
                        std::is_same_v<MsgType, ITCHAddOrderMPIDMessage>)
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
         else if constexpr (std::is_same_v<MsgType, ITCHAddOrderMPIDMessage>)
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
         else if constexpr (std::is_same_v<MsgType, ITCHCancelMessage>)
         {
            auto col_order_ref = std::make_shared<ColumnUInt64>();
            col_order_ref->Append(msg->order_ref);
            block.AppendColumn("order_ref", col_order_ref);

            auto col_remaining_quantity = std::make_shared<ColumnUInt32>();
            col_remaining_quantity->Append(msg->remaining_quantity);
            block.AppendColumn("remaining_quantity", col_remaining_quantity);
         }
         else if constexpr (std::is_same_v<MsgType, ITCHExecutedMessage>)
         {
            auto col_order_ref = std::make_shared<ColumnUInt64>();
            col_order_ref->Append(msg->order_ref);
            block.AppendColumn("order_ref", col_order_ref);

            auto col_executed_quantity = std::make_shared<ColumnUInt32>();
            col_executed_quantity->Append(msg->executed_quantity);
            block.AppendColumn("executed_quantity", col_executed_quantity);
         }
         else if constexpr (std::is_same_v<MsgType, ITCHExecutedWithPriceMessage>)
         {
            auto col_order_ref = std::make_shared<ColumnUInt64>();
            col_order_ref->Append(msg->order_ref);
            block.AppendColumn("order_ref", col_order_ref);

            auto col_executed_quantity = std::make_shared<ColumnUInt32>();
            col_executed_quantity->Append(msg->executed_quantity);
            block.AppendColumn("executed_quantity", col_executed_quantity);

            auto col_execution_price = std::make_shared<ColumnUInt32>();
            col_execution_price->Append(msg->execution_price);
            block.AppendColumn("executed_quantity", col_execution_price);
         }
         else if constexpr (std::is_same_v<MsgType, ITCHDeleteMessage>)
         {
            auto col_order_ref = std::make_shared<ColumnUInt64>();
            col_order_ref->Append(msg->order_ref);
            block.AppendColumn("order_ref", col_order_ref);
         }
         else if constexpr (std::is_same_v<MsgType, ITCHTradeMessage>)
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
         else if constexpr (std::is_same_v<MsgType, ITCHSystemEventMessage>)
         {
            auto col_event_code = std::make_shared<ColumnString>();
            col_event_code->Append(std::string(1,msg->event_code));
            block.AppendColumn("event_code", col_event_code);
         }

         client_->Insert("ITCH_Table", block); }, itchMsg.msg);
} */

void Store_DB::insert(const MessageWithVenue<NASDAQ::ITCHMessage> &itchMsg)
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

         if constexpr (std::is_same_v<MsgType, NASDAQ::ITCHAddOrderMessage> ||
                        std::is_same_v<MsgType, NASDAQ::ITCHAddOrderMPIDMessage>)
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
            block.AppendColumn("executed_quantity", col_execution_price);
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

         client_->Insert("ITCH_Table", block); }, itchMsg.msg);
}

// === INSERT SBE ===
void Store_DB::insert(const MessageWithVenue<SBEMessage> &sbeMsg)
{
   std::visit([this, venue = sbeMsg.venue](const auto *msg)
              {
        using MsgType = std::remove_pointer_t<decltype(msg)>;

        Block block;

        auto col_timestamp = std::make_shared<ColumnUInt64>();
        col_timestamp->Append(msg->header.version);
        block.AppendColumn("timestamp", col_timestamp);

        auto col_venue = std::make_shared<ColumnString>();
        col_venue->Append(venue_to_string(venue));
        block.AppendColumn("venue", col_venue);

        auto col_protocol = std::make_shared<ColumnString>();
        col_protocol->Append(protocol_to_string(Protocol::SBE));
        block.AppendColumn("protocol", col_protocol);

        auto col_template_id = std::make_shared<ColumnUInt16>();
        col_template_id->Append(msg->header.templateId);
        block.AppendColumn("template_id", col_template_id);

        if constexpr (std::is_same_v<MsgType, SBEAddOrderMessage>)
        {
            auto col_order_id = std::make_shared<ColumnUInt64>();
            col_order_id->Append(msg->orderId);
            block.AppendColumn("order_id", col_order_id);

            auto col_price = std::make_shared<ColumnUInt32>();
            col_price->Append(msg->price);
            block.AppendColumn("price", col_price);

            auto col_quantity = std::make_shared<ColumnUInt32>();
            col_quantity->Append(msg->quantity);
            block.AppendColumn("quantity", col_quantity);

            auto col_side = std::make_shared<ColumnUInt8>();
            col_side->Append(msg->side);
            block.AppendColumn("side", col_side);
        }
        else if constexpr (std::is_same_v<MsgType, SBEModifyOrderMessage>)
        {
           auto col_order_id = std::make_shared<ColumnUInt64>();
           col_order_id->Append(msg->orderId);
           block.AppendColumn("order_id", col_order_id);

           auto col_newQuantity = std::make_shared<ColumnUInt32>();
           col_newQuantity->Append(msg->newQuantity);
           block.AppendColumn("new_quantity", col_newQuantity);
        }
        else if constexpr (std::is_same_v<MsgType, SBEDeleteOrderMessage>)
        {
           auto col_order_id = std::make_shared<ColumnUInt64>();
           col_order_id->Append(msg->orderId);
           block.AppendColumn("order_id", col_order_id);
        }
        else if constexpr (std::is_same_v<MsgType, SBETradeMessage>)
        {
           auto col_order_id = std::make_shared<ColumnUInt64>();
           col_order_id->Append(msg->orderId);
           block.AppendColumn("order_id", col_order_id);

           auto col_trade_id = std::make_shared<ColumnUInt64>();
           col_trade_id->Append(msg->tradeId);
           block.AppendColumn("trade_id", col_trade_id);

           auto col_price = std::make_shared<ColumnUInt32>();
           col_price->Append(msg->price);
           block.AppendColumn("price", col_price);

           auto col_quantity = std::make_shared<ColumnUInt32>();
           col_quantity->Append(msg->quantity);
           block.AppendColumn("quantity", col_quantity);

           auto col_aggressor_side = std::make_shared<ColumnUInt8>();
           col_aggressor_side->Append(msg->aggressorSide);
           block.AppendColumn("aggressor_side", col_aggressor_side);
        }
        else if constexpr (std::is_same_v<MsgType, SBEMarketStatusMessage>)
        {
            auto col_instrument_id = std::make_shared<ColumnUInt64>();
            col_instrument_id->Append(msg->instrumentId);
            block.AppendColumn("instrument_id", col_instrument_id);

            auto col_market_state = std::make_shared<ColumnUInt8>();
            col_market_state->Append(msg->marketState);
            block.AppendColumn("market_state", col_market_state);
        }
        else if constexpr (std::is_same_v<MsgType, SBEInstrumentDefinitionMessage>)
        {
           std::string currency(reinterpret_cast<const char *>(msg->currencyCode), 3);

           auto col_instrument_id = std::make_shared<ColumnUInt64>();
           col_instrument_id->Append(msg->instrumentId);
           block.AppendColumn("instrument_id", col_instrument_id);

           auto col_lot_size = std::make_shared<ColumnUInt32>();
           col_lot_size->Append(msg->lotSize);
           block.AppendColumn("lot_size", col_lot_size);

           auto col_currencyCode = std::make_shared<ColumnString>();
           col_currencyCode->Append(currency);
           block.AppendColumn("curreny_code", col_currencyCode);
        }
        else if constexpr (std::is_same_v<MsgType, SBESequenceResetMessage>)
        {
            auto col_new_seq_no = std::make_shared<ColumnUInt64>();
            col_new_seq_no->Append(msg->newSeqNo);
            block.AppendColumn("new_seq_no", col_new_seq_no);
        }
        else if constexpr (std::is_same_v<MsgType, SBEHeartbeatMessage>)
        {
            auto col_timestamp_hb = std::make_shared<ColumnUInt64>();
            col_timestamp_hb->Append(msg->timestamp);
            block.AppendColumn("timestamp", col_timestamp_hb);
        }

        client_->Insert("SBE_Table", block); }, sbeMsg.msg);
}
// === INSERT ORDER ===
void Store_DB::insert(const Order *order)
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

   // Venue & Protocol
   auto col_venue = std::make_shared<ColumnString>();
   col_venue->Append(venue_to_string(order->venue));
   block.AppendColumn("venue", col_venue);

   auto col_protocol = std::make_shared<ColumnString>();
   col_protocol->Append(protocol_to_string(order->protocol));
   block.AppendColumn("protocol", col_protocol);

   // Order ve Client ID
   auto col_order_id = std::make_shared<ColumnUInt64>();
   col_order_id->Append(order->order_id);
   block.AppendColumn("order_id", col_order_id);

   auto col_client_order_id = std::make_shared<ColumnUInt64>();
   col_client_order_id->Append(order->client_order_id);
   block.AppendColumn("client_order_id", col_client_order_id);

   // Fiyat ve Miktarlar
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

   // Symbol (fixed 8-byte array -> string)
   auto col_symbol = std::make_shared<ColumnString>();
   col_symbol->Append(symbol);
   block.AppendColumn("symbol", col_symbol);

   // Instrument ID
   auto col_instrument_id = std::make_shared<ColumnUInt32>();
   col_instrument_id->Append(order->instrument_id);
   block.AppendColumn("instrument_id", col_instrument_id);

   // Side (0 = Buy, 1 = Sell)
   auto col_side = std::make_shared<ColumnUInt8>();
   col_side->Append(static_cast<uint8_t>(order->side));
   block.AppendColumn("side", col_side);

   // Status (uint8 enum)
   auto col_status = std::make_shared<ColumnString>();
   col_status->Append(status_to_string(order->status));
   block.AppendColumn("status", col_status);

   // Mesaj Tipi (uint16)
   auto col_message_type = std::make_shared<ColumnUInt16>();
   col_message_type->Append(order->message_type);
   block.AppendColumn("message_type", col_message_type);

   // Taktik parametreler
   auto col_time_in_force = std::make_shared<ColumnUInt8>();
   col_time_in_force->Append(static_cast<uint8_t>(order->time_in_force));
   block.AppendColumn("time_in_force", col_time_in_force);

   auto col_order_type = std::make_shared<ColumnUInt8>();
   col_order_type->Append(static_cast<uint8_t>(order->order_type));
   block.AppendColumn("order_type", col_order_type);

   auto col_priority_level = std::make_shared<ColumnUInt8>();
   col_priority_level->Append(order->priority_level);
   block.AppendColumn("priority_level", col_priority_level);

   client_->Insert("OrdersTable", block);
}
