#include "Parser_OUCH_NASDAQ.h"
#include "Endian.h"

#include <cstring>

Parser_OUCH_NASDAQ::Parser_OUCH_NASDAQ() noexcept
{
   ouch_pools_.initialize_all();
}

const std::array<Parser_OUCH_NASDAQ::MessageHandlerFunc, Parser_OUCH_NASDAQ::MAX_MESSAGES> Parser_OUCH_NASDAQ::makeMessageHandlersLookup() noexcept
{
   static std::array<MessageHandlerFunc, MAX_MESSAGES> handlers{};

   handlers[0] = [](Parser_OUCH_NASDAQ &parser, const char *data, NASDAQ::OUCHMessage &msg) noexcept
   {
      NASDAQ::RX::OUCHSystemEventMessage *m = nullptr;
      parser.ouch_pools_.get_pool<NASDAQ::RX::OUCHSystemEventMessage>().acquire(m);

      m->timestamp = Endian::read_u64_be(data + 1);
      m->message_type = data[0];
      m->event_code = data[9];

      msg = m;
   };
   
   handlers[1] = [](Parser_OUCH_NASDAQ &parser, const char *data, NASDAQ::OUCHMessage &msg) noexcept
   {
      NASDAQ::RX::OUCHOrderAcceptedMessage *m = nullptr;
      parser.ouch_pools_.get_pool<NASDAQ::RX::OUCHOrderAcceptedMessage>().acquire(m);

      m->timestamp = Endian::read_u64_be(data + 1);
      m->user_ref_num = Endian::read_u32_be(data + 9);
      m->price = Endian::read_i64_be(data + 26);
      m->order_reference_number= Endian::read_u64_be(data + 36);
      m->quantity = Endian::read_u32_be(data + 14);
      m->appendage_length = Endian::read_u16_be(data + 62);
      m->message_type = data[0];
      m->side = data[13];
      std::memcpy(&m->symbol, data + 18, 8);
      m->time_in_force = data[34];
      m->display = data[35];
      m->capacity = data[44];
      m->intermarket_sweep_eligibility = data[45];
      m->cross_type = data[46];
      m->order_state = data[47];
      std::memcpy(&m->cl_ord_id, data + 48, 14);

      msg = m;
   };

   handlers[2] = [](Parser_OUCH_NASDAQ &parser, const char *data, NASDAQ::OUCHMessage &msg) noexcept
   {
      NASDAQ::RX::OUCHOrderReplacedMessage *m = nullptr;
      parser.ouch_pools_.get_pool<NASDAQ::RX::OUCHOrderReplacedMessage>().acquire(m);

      m->timestamp = Endian::read_u64_be(data + 1);
      m->price = Endian::read_i64_be(data + 30);
      m->order_reference_number = Endian::read_u64_be(data + 40);
      m->orig_user_ref_num = Endian::read_u32_be(data + 9);
      m->user_ref_num = Endian::read_u32_be(data + 13);
      m->quantity = Endian::read_u32_be(data + 18);
      m->appendage_length = Endian::read_u16_be(data + 66);
      m->message_type = data[0];
      m->side = data[17];
      std::memcpy(&m->symbol, data + 22, 8);
      m->time_in_force = data[38];
      m->order_state = data[51];
      std::memcpy(&m->cl_ord_id, data + 52, 14);
      m->display = data[39];
      m->capacity = data[48];
      m->intermarket_sweep_eligibility = data[49];
      m->cross_type = data[50];
     
      msg = m;
   };

   handlers[3] = [](Parser_OUCH_NASDAQ &parser, const char *data, NASDAQ::OUCHMessage &msg) noexcept
   {
      NASDAQ::RX::OUCHOrderCancelledMessage *m = nullptr;
      parser.ouch_pools_.get_pool<NASDAQ::RX::OUCHOrderCancelledMessage>().acquire(m);

      m->timestamp = Endian::read_u64_be(data + 1);
      m->user_ref_num = Endian::read_u32_be(data + 9);
      m->quantity = Endian::read_u32_be(data + 13);
      m->appendage_length = Endian::read_u16_be(data + 18);
      m->message_type = data[0];
      m->reason = data[17];

      msg = m;
   };

   handlers[4] = [](Parser_OUCH_NASDAQ &parser, const char *data, NASDAQ::OUCHMessage &msg) noexcept
   {
      NASDAQ::RX::OUCHOrderExecutedMessage *m = nullptr;
      parser.ouch_pools_.get_pool<NASDAQ::RX::OUCHOrderExecutedMessage>().acquire(m);

      m->timestamp = Endian::read_u64_be(data + 1);
      m->price = Endian::read_i64_be(data + 17);
      m->match_number = Endian::read_u64_be(data + 26);
      m->user_ref_num = Endian::read_u32_be(data + 9);
      m->quantity = Endian::read_u32_be(data + 13);
      m->appendage_length = Endian::read_u16_be(data + 34);
      m->message_type = data[0];
      m->liquidity_flag = data[25];

      msg = m;
   };

   handlers[5] = [](Parser_OUCH_NASDAQ &parser, const char *data, NASDAQ::OUCHMessage &msg) noexcept
   {
      NASDAQ::RX::OUCHOrderRejectedMessage *m = nullptr;
      parser.ouch_pools_.get_pool<NASDAQ::RX::OUCHOrderRejectedMessage>().acquire(m);

      m->timestamp = Endian::read_u64_be(data + 1);
      m->user_ref_num = Endian::read_u32_be(data + 9);
      m->reason = Endian::read_u16_be(data + 13);
      m->appendage_length = Endian::read_u16_be(data + 29);
      m->message_type = data[0];
      std::memcpy(&m->cl_ord_id, data + 15, 14);

      msg = m;
   };

   handlers[6] = [](Parser_OUCH_NASDAQ &parser, const char *data, NASDAQ::OUCHMessage &msg) noexcept
   {
      NASDAQ::RX::OUCHBrokenTradeMessage *m = nullptr;
      parser.ouch_pools_.get_pool<NASDAQ::RX::OUCHBrokenTradeMessage>().acquire(m);

      m->timestamp = Endian::read_u64_be(data + 1);
      m->match_number = Endian::read_u64_be(data + 13);
      m->user_ref_num = Endian::read_u32_be(data + 9);
      m->appendage_length = Endian::read_u16_be(data + 36);
      m->message_type = data[0];
      m->reason = data[21];
      std::memcpy(&m->cl_ord_id, data + 22, 14);

      msg = m;
   };

   handlers[7] = [](Parser_OUCH_NASDAQ &parser, const char *data, NASDAQ::OUCHMessage &msg) noexcept
   {
      NASDAQ::RX::OUCHOrderModifiedMessage *m = nullptr;
      parser.ouch_pools_.get_pool<NASDAQ::RX::OUCHOrderModifiedMessage>().acquire(m);

      m->timestamp = Endian::read_u64_be(data + 1);
      m->user_ref_num = Endian::read_u32_be(data + 9);
      m->quantity = Endian::read_u32_be(data + 14);
      m->appendage_length = Endian::read_u16_be(data + 18);
      m->message_type = data[0];
      m->side = data[13];

      msg = m;
   };

   handlers[8] = [](Parser_OUCH_NASDAQ &parser, const char *data, NASDAQ::OUCHMessage &msg) noexcept
   {
      NASDAQ::RX::OUCHCancelPendingMessage *m = nullptr;
      parser.ouch_pools_.get_pool<NASDAQ::RX::OUCHCancelPendingMessage>().acquire(m);

      m->timestamp = Endian::read_u64_be(data + 1);
      m->user_ref_num = Endian::read_u32_be(data + 9);
      m->appendage_length = Endian::read_u16_be(data + 13);
      m->message_type = data[0];

      msg = m;
   };

   handlers[9] = [](Parser_OUCH_NASDAQ &parser, const char *data, NASDAQ::OUCHMessage &msg) noexcept
   {
      NASDAQ::RX::OUCHCancelRejectMessage *m = nullptr;
      parser.ouch_pools_.get_pool<NASDAQ::RX::OUCHCancelRejectMessage>().acquire(m);

      m->timestamp = Endian::read_u64_be(data + 1);
      m->user_ref_num = Endian::read_u32_be(data + 9);
      m->appendage_length = Endian::read_u16_be(data + 13);
      m->message_type = data[0];

      msg = m;
   };

   handlers[10] = [](Parser_OUCH_NASDAQ &parser, const char *data, NASDAQ::OUCHMessage &msg) noexcept
   {
      NASDAQ::RX::OUCHAccountQueryResponse *m = nullptr;
      parser.ouch_pools_.get_pool<NASDAQ::RX::OUCHAccountQueryResponse>().acquire(m);

      m->timestamp = Endian::read_u64_be(data + 1);
      m->next_user_ref_num = Endian::read_u32_be(data + 9);
      m->appendage_length = Endian::read_u16_be(data + 13);
      m->message_type = data[0];

      msg = m;
   };

   return handlers;
}

const std::array<Parser_OUCH_NASDAQ::MessageHandlerFunc, Parser_OUCH_NASDAQ::MAX_MESSAGES> Parser_OUCH_NASDAQ::MessageHandlers = makeMessageHandlersLookup();
