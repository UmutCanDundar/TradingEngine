#include "Parser_OUCH_BIST.h"
#include "Endian.h"

#include <cstring>



Parser_OUCH_BIST::Parser_OUCH_BIST() noexcept
{
   ouch_pools_.initialize_all();
}

const std::array<Parser_OUCH_BIST::MessageHandlerFunc, Parser_OUCH_BIST::MAX_MESSAGES> Parser_OUCH_BIST::makeMessageHandlersLookup() noexcept
{
   static std::array<MessageHandlerFunc, MAX_MESSAGES> handlers{};

   handlers[0] = [](Parser_OUCH_BIST &parser, const char *data, BIST::OUCHMessage &msg) noexcept
   {
      BIST::OUT::OUCHOrderAcceptedMessage *m = nullptr;
      parser.ouch_pools_.get_pool<BIST::OUT::OUCHOrderAcceptedMessage>().acquire(m);

      m->timestamp = Endian::read_u64_be(data + 1);
      m->order_id = Endian::read_u64_be(data + 28);
      m->quantity = Endian::read_u64_be(data + 36);
      m->pre_trade_qty = Endian::read_u64_be(data + 114);
      m->display_qty = Endian::read_u64_be(data + 122);
      m->order_book_id = Endian::read_u32_be(data + 23);
      m->price = Endian::read_i32_be(data + 44);
      m->message_type = data[0];
      std::memcpy(&m->order_token, data + 9, 14);
      m->side = data[27];
      m->time_in_force = data[48];
      std::memcpy(&m->client_account, data + 50, 16);
      std::memcpy(&m->customer_info, data + 67, 15);
      std::memcpy(&m->exchange_info, data + 82, 32);
      m->open_close = data[49];
      m->order_state = data[66];
      m->client_category = data[130];
      m->offhours = data[131];
      m->smp_level = data[132];
      m->smp_method = data[133];
      std::memcpy(&m->smp_ID, data + 134, 3);

      msg = m;
   };

   handlers[1] = [](Parser_OUCH_BIST &parser, const char *data, BIST::OUCHMessage &msg) noexcept
   {
      BIST::OUT::OUCHOrderRejectedMessage *m = nullptr;
      parser.ouch_pools_.get_pool<BIST::OUT::OUCHOrderRejectedMessage>().acquire(m);

      m->timestamp = Endian::read_u64_be(data + 1);
      m->reject_code = Endian::read_i32_be(data + 23);
      m->message_type = data[0];
      std::memcpy(&m->order_token, data + 9, 14);
     
      msg = m;
   };

   handlers[2] = [](Parser_OUCH_BIST &parser, const char *data, BIST::OUCHMessage &msg) noexcept
   {
      BIST::OUT::OUCHOrderReplacedMessage *m = nullptr;
      parser.ouch_pools_.get_pool<BIST::OUT::OUCHOrderReplacedMessage>().acquire(m);

      m->timestamp = Endian::read_u64_be(data + 1);
      m->order_id = Endian::read_u64_be(data + 42);
      m->quantity = Endian::read_u64_be(data + 50);
      m->pre_trade_qty = Endian::read_u64_be(data + 128);
      m->order_book_id = Endian::read_u32_be(data + 37);
      m->price = Endian::read_i32_be(data + 58);
      m->message_type = data[0];
      std::memcpy(&m->previous_order_token, data + 23, 14);
      m->side = data[41];
      m->time_in_force = data[62];
      m->open_close = data[63];
      m->order_state = data[80];
      m->client_category = data[144];
      m->display_qty = Endian::read_u64_be(data + 136);
      std::memcpy(&m->order_token, data + 9, 14);
      std::memcpy(&m->exchange_info, data + 96, 32);
      std::memcpy(&m->client_account, data + 64, 16);
      std::memcpy(&m->customer_info, data + 81, 15);

      msg = m;
   };

   handlers[3] = [](Parser_OUCH_BIST &parser, const char *data, BIST::OUCHMessage &msg) noexcept
   {
      BIST::OUT::OUCHOrderCancelledMessage *m = nullptr;
      parser.ouch_pools_.get_pool<BIST::OUT::OUCHOrderCancelledMessage>().acquire(m);

      m->timestamp = Endian::read_u64_be(data + 1);
      m->order_id = Endian::read_u64_be(data + 28);
      m->order_book_id = Endian::read_u32_be(data + 23);
      m->message_type = data[0];
      std::memcpy(&m->order_token, data + 9, 14);
      m->side = data[27];
      m->reason = data[36];

      msg = m;
   };

   handlers[4] = [](Parser_OUCH_BIST &parser, const char *data, BIST::OUCHMessage &msg) noexcept
   {
      BIST::OUT::OUCHOrderExecutedMessage *m = nullptr;
      parser.ouch_pools_.get_pool<BIST::OUT::OUCHOrderExecutedMessage>().acquire(m);

      m->timestamp = Endian::read_u64_be(data + 1);
      m->traded_quantity = Endian::read_u64_be(data + 27);
      m->order_book_id = Endian::read_u32_be(data + 23);
      m->trade_price = Endian::read_i32_be(data + 35);
      m->message_type = data[0];
      std::memcpy(&m->order_token, data + 9, 14);
      std::memcpy(&m->match_id, data + 39, 12);
      m->client_category = data[51];
      std::memcpy(&m->reserved, data + 52, 16);

      msg = m;
   };

   return handlers;
}

const std::array<Parser_OUCH_BIST::MessageHandlerFunc, Parser_OUCH_BIST::MAX_MESSAGES> Parser_OUCH_BIST::MessageHandlers = makeMessageHandlersLookup();
