#include "Parser_ITCH_BIST.h"
#include "Endian.h"

#include <cstring>

Parser_ITCH_BIST::Parser_ITCH_BIST() noexcept
{
   itch_pools_.initialize_all();
}

const std::array<Parser_ITCH_BIST::MessageHandlerFunc, Parser_ITCH_BIST::MAX_MESSAGES> Parser_ITCH_BIST::makeMessageHandlersLookup() noexcept
{
   static std::array<MessageHandlerFunc, MAX_MESSAGES> handlers{};

   handlers[0] = [](Parser_ITCH_BIST &parser, const char *data, BIST::ITCHMessage &msg) noexcept
   {
      BIST::ITCHSecondsMessage *m = nullptr;
      parser.itch_pools_.get_pool<BIST::ITCHSecondsMessage>().acquire(m);

      m->second = Endian::read_u32_be(data + 1);
      m->message_type = data[0];

      msg = m;
   };

   handlers[1] = [](Parser_ITCH_BIST &parser, const char *data, BIST::ITCHMessage &msg) noexcept
   {
      BIST::ITCHOrderBookDirectoryMessage *m = nullptr;
      parser.itch_pools_.get_pool<BIST::ITCHOrderBookDirectoryMessage>().acquire(m);

      m->nominal_value = Endian::read_u64_be(data + 105);
      m->timestamp_ns = Endian::read_u32_be(data + 1);
      m->order_book_id = Endian::read_u32_be(data + 5);
      m->odd_lot_size = Endian::read_u32_be(data + 93);
      m->round_lot_size = Endian::read_u32_be(data + 97);
      m->block_lot_size = Endian::read_u32_be(data + 101);
      m->underlying_order_book_id = Endian::read_u32_be(data + 114);
      m->strike_price = Endian::read_i32_be(data + 118);
      m->expiration_date = Endian::read_u32_be(data + 122);
      m->decimals_in_price = Endian::read_u16_be(data + 89);
      m->decimals_in_nominal = Endian::read_u16_be(data + 91);
      m->decimals_in_strike_price = Endian::read_u16_be(data + 126);
      m->financial_product = data[85];
      m->number_of_legs = data[113];
      m->put_or_call = data[128];
      m->ranking_type = data[129];
      m->message_type = data[0];
      std::memcpy(&m->isin, data + 73, 12);
      std::memcpy(&m->symbol, data + 9, 32);
      std::memcpy(&m->long_name, data + 41, 32);
      std::memcpy(&m->trading_currency, data + 86, 3);
     
      msg = m;
   };

   handlers[2] = [](Parser_ITCH_BIST &parser, const char *data, BIST::ITCHMessage &msg) noexcept
   {
      BIST::ITCHCombinationOrderBookLegMessage *m = nullptr;
      parser.itch_pools_.get_pool<BIST::ITCHCombinationOrderBookLegMessage>().acquire(m);

      m->timestamp_ns = Endian::read_u32_be(data + 1);
      m->combination_order_book_id = Endian::read_u32_be(data + 5);
      m->leg_order_book_id = Endian::read_u32_be(data + 9);
      m->leg_ratio = Endian::read_u32_be(data + 14);
      m->message_type = data[0];
      m->leg_side = data[13];

      msg = m;
   };

   handlers[3] = [](Parser_ITCH_BIST &parser, const char *data, BIST::ITCHMessage &msg) noexcept
   {
      BIST::ITCHTickSizeTableEntryMessage *m = nullptr;
      parser.itch_pools_.get_pool<BIST::ITCHTickSizeTableEntryMessage>().acquire(m);

      m->tick_size = Endian::read_u64_be(data + 9);
      m->timestamp_ns = Endian::read_u32_be(data + 1);
      m->order_book_id = Endian::read_u32_be(data + 5);
      m->price_from = Endian::read_i32_be(data + 17);
      m->price_to = Endian::read_i32_be(data + 21);
      m->message_type = data[0];

      msg = m;
   };

   handlers[4] = [](Parser_ITCH_BIST &parser, const char *data, BIST::ITCHMessage &msg) noexcept
   {
      BIST::ITCHShortSellStatusMessage *m = nullptr;
      parser.itch_pools_.get_pool<BIST::ITCHShortSellStatusMessage>().acquire(m);

      m->timestamp_ns = Endian::read_u32_be(data + 1);
      m->order_book_id = Endian::read_u32_be(data + 5);
      m->short_sale_restriction = data[9];
      m->message_type = data[0];

      msg = m;
   };

   handlers[5] = [](Parser_ITCH_BIST &parser, const char *data, BIST::ITCHMessage &msg) noexcept
   {
      BIST::ITCHSystemEventMessage *m = nullptr;
      parser.itch_pools_.get_pool<BIST::ITCHSystemEventMessage>().acquire(m);

      m->timestamp_ns = Endian::read_u32_be(data + 1);
      m->message_type = data[0];
      m->event_code = data[5];
      
      msg = m;
   };

   handlers[6] = [](Parser_ITCH_BIST &parser, const char *data, BIST::ITCHMessage &msg) noexcept
   {
      BIST::ITCHOrderBookStateMessage *m = nullptr;
      parser.itch_pools_.get_pool<BIST::ITCHOrderBookStateMessage>().acquire(m);

      m->timestamp_ns = Endian::read_u32_be(data + 1);
      m->order_book_id = Endian::read_u32_be(data + 5);
      m->message_type = data[0];
      std::memcpy(&m->state_name, data + 9, 20);

      msg = m;
   };

   handlers[7] = [](Parser_ITCH_BIST &parser, const char *data, BIST::ITCHMessage &msg) noexcept
   {
      BIST::ITCHAddOrderMessage *m = nullptr;
      parser.itch_pools_.get_pool<BIST::ITCHAddOrderMessage>().acquire(m);

      m->order_id = Endian::read_u64_be(data + 5);
      m->quantity = Endian::read_u64_be(data + 22);
      m->ranking_time = Endian::read_u64_be(data + 37);
      m->timestamp_ns = Endian::read_u32_be(data + 1);
      m->order_book_id = Endian::read_u32_be(data + 13);
      m->ranking_seq_number = Endian::read_u32_be(data + 18);
      m->price = Endian::read_i32_be(data + 30);
      m->order_attributes = Endian::read_u16_be(data + 34);
      m->lot_type = data[36];
      m->message_type = data[0];
      m->side = data[17];

      msg = m;
   };

   handlers[8] = [](Parser_ITCH_BIST &parser, const char *data, BIST::ITCHMessage &msg) noexcept
   {
      BIST::ITCHOrderExecutedMessage *m = nullptr;
      parser.itch_pools_.get_pool<BIST::ITCHOrderExecutedMessage>().acquire(m);

      m->order_id = Endian::read_u64_be(data + 5);
      m->executed_quantity = Endian::read_u64_be(data + 18);
      m->match_id = Endian::read_u64_be(data + 26);
      m->timestamp_ns = Endian::read_u32_be(data + 1);
      m->order_book_id = Endian::read_u32_be(data + 13);
      m->combo_group_id = Endian::read_u32_be(data + 34);
      m->message_type = data[0];
      m->side = data[17];
      std::memcpy(&m->reserved1, data + 38, 7);
      std::memcpy(&m->reserved2, data + 45, 7);

      msg = m;
   };

   handlers[9] = [](Parser_ITCH_BIST &parser, const char *data, BIST::ITCHMessage &msg) noexcept
   {
      BIST::ITCHOrderExecutedWithPriceMessage *m = nullptr;
      parser.itch_pools_.get_pool<BIST::ITCHOrderExecutedWithPriceMessage>().acquire(m);

      m->order_id = Endian::read_u64_be(data + 5);
      m->executed_quantity = Endian::read_u64_be(data + 18);
      m->match_id = Endian::read_u64_be(data + 26);
      m->timestamp_ns = Endian::read_u32_be(data + 1);
      m->order_book_id = Endian::read_u32_be(data + 13);
      m->combo_group_id = Endian::read_u32_be(data + 34);
      m->trade_price = Endian::read_i32_be(data + 52);
      m->message_type = data[0];
      m->side = data[17];
      std::memcpy(&m->reserved1, data + 38, 7);
      std::memcpy(&m->reserved2, data + 45, 7);
      m->occurred_at_cross = data[56];
      m->printable = data[57];

      msg = m;
   };

   handlers[10] = [](Parser_ITCH_BIST &parser, const char *data, BIST::ITCHMessage &msg) noexcept
   {
      BIST::ITCHOrderDeleteMessage *m = nullptr;
      parser.itch_pools_.get_pool<BIST::ITCHOrderDeleteMessage>().acquire(m);

      m->order_id = Endian::read_u64_be(data + 5);
      m->timestamp_ns = Endian::read_u32_be(data + 1);
      m->order_book_id = Endian::read_u32_be(data + 13);
      m->message_type = data[0];
      m->side = data[17];

      msg = m;
   };

   handlers[11] = [](Parser_ITCH_BIST &parser, const char *data, BIST::ITCHMessage &msg) noexcept
   {
      BIST::ITCHOrderBookFlushMessage *m = nullptr;
      parser.itch_pools_.get_pool<BIST::ITCHOrderBookFlushMessage>().acquire(m);

      m->timestamp_ns = Endian::read_u32_be(data + 1);
      m->order_book_id = Endian::read_u32_be(data + 5);
      m->message_type = data[0];

      msg = m;
   };

   handlers[12] = [](Parser_ITCH_BIST &parser, const char *data, BIST::ITCHMessage &msg) noexcept
   {
      BIST::ITCHTradeMessage *m = nullptr;
      parser.itch_pools_.get_pool<BIST::ITCHTradeMessage>().acquire(m);

      m->match_id = Endian::read_u64_be(data + 5);
      m->quantity = Endian::read_u64_be(data + 18);
      m->timestamp_ns = Endian::read_u32_be(data + 1);
      m->combo_group_id = Endian::read_u32_be(data + 13);
      m->order_book_id = Endian::read_u32_be(data + 26);
      m->trade_price = Endian::read_i32_be(data + 30);
      m->message_type = data[0];
      m->side = data[17];
      std::memcpy(&m->reserved1, data + 34, 7);
      std::memcpy(&m->reserved2, data + 41, 7);
      m->printable = data[48];
      m->occurred_at_cross = data[49];

      msg = m;
   };

   handlers[13] = [](Parser_ITCH_BIST &parser, const char *data, BIST::ITCHMessage &msg) noexcept
   {
      BIST::ITCHEquilibriumPriceUpdateMessage *m = nullptr;
      parser.itch_pools_.get_pool<BIST::ITCHEquilibriumPriceUpdateMessage>().acquire(m);

      m->available_bid_quantity = Endian::read_u64_be(data + 9);
      m->available_ask_quantity = Endian::read_u64_be(data + 17);
      m->best_bid_quantity = Endian::read_u64_be(data + 37);
      m->best_ask_quantity = Endian::read_u64_be(data + 45);
      m->timestamp_ns = Endian::read_u32_be(data + 1);
      m->order_book_id = Endian::read_u32_be(data + 5);
      m->equilibrium_price = Endian::read_i32_be(data + 25);
      m->best_bid_price = Endian::read_i32_be(data + 29);
      m->best_ask_price = Endian::read_i32_be(data + 33);
      m->message_type = data[0];

      msg = m;
   };

   return handlers;
}

const std::array<Parser_ITCH_BIST::MessageHandlerFunc, Parser_ITCH_BIST::MAX_MESSAGES> Parser_ITCH_BIST::MessageHandlers = makeMessageHandlersLookup();
