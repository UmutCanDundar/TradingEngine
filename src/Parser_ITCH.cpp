#include "Parser_ITCH.h"
#include "Endian.h"

MessagePools<ITCHMessageTypes> Parser_ITCH::itch_pools_;

Parser_ITCH::Parser_ITCH() noexcept
{
   itch_pools_.initialize_all();
}

std::array<Parser_ITCH::MessageHandlerFunc, Parser_ITCH::MAX_MESSAGES> Parser_ITCH::makeMessageHandlersLookup() noexcept
{
   alignas(64) std::array<MessageHandlerFunc, MAX_MESSAGES> handlers{};

   handlers[0] = [](const char *data, ITCHMessage &msg) noexcept
   {
      ITCHAddOrderMessage *m = nullptr;
      itch_pools_.get_pool<ITCHAddOrderMessage>().acquire(m);

      m->timestamp = Endian::read_u64_be(data + 5);
      m->order_ref = Endian::read_u64_be(data + 11);
      m->shares = Endian::read_u32_be(data + 20);
      m->price = Endian::read_u32_be(data + 32);
      m->stock_locate = Endian::read_u16_be(data + 1);
      m->tracking_number = Endian::read_u16_be(data + 3);
      m->message_type = data[0];
      m->side = data[19];
      std::memcpy(&m->stock, data + 24, 8);

      msg = m;
   };

   handlers[1] = [](const char *data, ITCHMessage &msg) noexcept
   {
      ITCHAddOrderMPIDMessage *m = nullptr;
      itch_pools_.get_pool<ITCHAddOrderMPIDMessage>().acquire(m);

      m->timestamp = Endian::read_u64_be(data + 5);
      m->order_ref = Endian::read_u64_be(data + 11);
      m->shares = Endian::read_u32_be(data + 20);
      m->price = Endian::read_u32_be(data + 32);
      m->stock_locate = Endian::read_u16_be(data + 1);
      m->tracking_number = Endian::read_u16_be(data + 3);
      m->message_type = data[0];
      m->side = data[19];
      std::memcpy(&m->stock, data + 24, 8);
      std::memcpy(&m->mpid, data + 36, 4);
      msg = m;
   };

   handlers[2] = [](const char *data, ITCHMessage &msg) noexcept
   {
      ITCHCancelMessage *m = nullptr;
      itch_pools_.get_pool<ITCHCancelMessage>().acquire(m);

      m->timestamp = Endian::read_u64_be(data + 5);
      m->order_ref = Endian::read_u64_be(data + 11);
      m->cancelled_shares = Endian::read_u32_be(data + 19);
      m->stock_locate = Endian::read_u16_be(data + 1);
      m->tracking_number = Endian::read_u16_be(data + 3);
      m->message_type = data[0];

      msg = m;
   };

   handlers[3] = [](const char *data, ITCHMessage &msg) noexcept
   {
      ITCHExecutedMessage *m = nullptr;
      itch_pools_.get_pool<ITCHExecutedMessage>().acquire(m);

      m->timestamp = Endian::read_u64_be(data + 5);
      m->order_ref = Endian::read_u64_be(data + 11);
      m->match_number = Endian::read_u64_be(data + 23);
      m->executed_shares = Endian::read_u32_be(data + 19);
      m->stock_locate = Endian::read_u16_be(data + 1);
      m->tracking_number = Endian::read_u16_be(data + 3);
      m->message_type = data[0];

      msg = m;
   };

   handlers[4] = [](const char *data, ITCHMessage &msg) noexcept
   {
      ITCHExecutedWithPriceMessage *m = nullptr;
      itch_pools_.get_pool<ITCHExecutedWithPriceMessage>().acquire(m);

      m->timestamp = Endian::read_u64_be(data + 5);
      m->order_ref = Endian::read_u64_be(data + 11);
      m->match_number = Endian::read_u64_be(data + 23);
      m->executed_shares = Endian::read_u32_be(data + 19);
      m->execution_price = Endian::read_u32_be(data + 32);
      m->stock_locate = Endian::read_u16_be(data + 1);
      m->tracking_number = Endian::read_u16_be(data + 3);
      m->message_type = data[0];
      m->printable = data[31];

      msg = m;
   };

   handlers[5] = [](const char *data, ITCHMessage &msg) noexcept
   {
      ITCHDeleteMessage *m = nullptr;
      itch_pools_.get_pool<ITCHDeleteMessage>().acquire(m);

      m->timestamp = Endian::read_u64_be(data + 5);
      m->order_ref = Endian::read_u64_be(data + 11);
      m->stock_locate = Endian::read_u16_be(data + 1);
      m->tracking_number = Endian::read_u16_be(data + 3);
      m->message_type = data[0];

      msg = m;
   };

   handlers[6] = [](const char *data, ITCHMessage &msg) noexcept
   {
      ITCHReplaceMessage *m = nullptr;
      itch_pools_.get_pool<ITCHReplaceMessage>().acquire(m);

      m->timestamp = Endian::read_u64_be(data + 5);
      m->order_ref = Endian::read_u64_be(data + 11);
      m->new_order_ref = Endian::read_u64_be(data + 19);
      m->shares = Endian::read_u32_be(data + 27);
      m->price = Endian::read_u32_be(data + 31);
      m->stock_locate = Endian::read_u16_be(data + 1);
      m->tracking_number = Endian::read_u16_be(data + 3);
      m->message_type = data[0];

      msg = m;
   };

   handlers[7] = [](const char *data, ITCHMessage &msg) noexcept
   {
      ITCHTradeMessage *m = nullptr;
      itch_pools_.get_pool<ITCHTradeMessage>().acquire(m);

      m->timestamp = Endian::read_u64_be(data + 5);
      m->order_ref = Endian::read_u64_be(data + 11);
      m->match_number = Endian::read_u64_be(data + 28);
      m->shares = Endian::read_u32_be(data + 20);
      m->price = Endian::read_u32_be(data + 24);
      m->stock_locate = Endian::read_u16_be(data + 1);
      m->tracking_number = Endian::read_u16_be(data + 3);
      m->message_type = data[0];
      m->side = data[19];
      m->cross_type = data[36];

      msg = m;
   };

   handlers[8] = [](const char *data, ITCHMessage &msg) noexcept
   {
      ITCHSystemEventMessage *m = nullptr;
      itch_pools_.get_pool<ITCHSystemEventMessage>().acquire(m);

      m->timestamp = Endian::read_u64_be(data + 5);
      m->stock_locate = Endian::read_u16_be(data + 1);
      m->tracking_number = Endian::read_u16_be(data + 3);
      m->message_type = data[0];
      m->event_code = data[11];

      msg = m;
   };

   handlers[9] = [](const char *data, ITCHMessage &msg) noexcept
   {
      ITCHStockDirectoryMessage *m = nullptr;
      itch_pools_.get_pool<ITCHStockDirectoryMessage>().acquire(m);

      m->timestamp = Endian::read_u64_be(data + 5);
      m->round_lot_size = Endian::read_u32_be(data + 21);
      m->etp_leverage_factor = Endian::read_u32_be(data + 34);
      m->stock_locate = Endian::read_u16_be(data + 1);
      m->tracking_number = Endian::read_u16_be(data + 3);
      m->message_type = data[0];
      std::memcpy(&m->stock, data + 11, 8);
      m->market_category = data[19];
      m->financial_status_indicator = data[20];
      m->round_lots_only = data[25];
      m->issue_classification = data[26];
      std::memcpy(&m->issue_sub_type, data + 27, 2);
      m->authenticity = data[29];
      m->short_sale_threshold_indicator = data[30];
      m->ipo_flag = data[31];
      m->luld_reference_price_tier = data[32];
      m->etp_flag = data[33];
      m->inverse_indicator = data[38];

      msg = m;
   };

   handlers[10] = [](const char *data, ITCHMessage &msg) noexcept
   {
      ITCHTradingStateMessage *m = nullptr;
      itch_pools_.get_pool<ITCHTradingStateMessage>().acquire(m);

      m->timestamp = Endian::read_u64_be(data + 5);
      m->stock_locate = Endian::read_u16_be(data + 1);
      m->tracking_number = Endian::read_u16_be(data + 3);
      m->message_type = data[0];
      std::memcpy(&m->stock, data + 11, 8);
      m->trading_state = data[19];
      m->reserved = data[20];
      std::memcpy(&m->reason, data + 21, 4);

      msg = m;
   };

   return handlers;
}

std::array<Parser_ITCH::MessageHandlerFunc, Parser_ITCH::MAX_MESSAGES> Parser_ITCH::MessageHandlers = makeMessageHandlersLookup();
