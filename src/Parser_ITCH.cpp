#include "Parser_ITCH.h"

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

      std::memcpy(&m->message_type, data, 1);
      std::memcpy(&m->timestamp, data + 1, 8);
      std::memcpy(&m->order_ref, data + 9, 8);
      std::memcpy(&m->side, data + 17, 1);
      std::memcpy(&m->quantity, data + 18, 4);
      std::memcpy(&m->stock, data + 22, 8);
      std::memcpy(&m->price, data + 30, 4);
      msg = m;
   };

   handlers[1] = [](const char *data, ITCHMessage &msg) noexcept
   {
      ITCHAddOrderMPIDMessage *m = nullptr;
      itch_pools_.get_pool<ITCHAddOrderMPIDMessage>().acquire(m);

      std::memcpy(&m->message_type, data, 1);
      std::memcpy(&m->timestamp, data + 1, 8);
      std::memcpy(&m->order_ref, data + 9, 8);
      std::memcpy(&m->side, data + 17, 1);
      std::memcpy(&m->quantity, data + 18, 4);
      std::memcpy(&m->stock, data + 22, 8);
      std::memcpy(&m->price, data + 30, 4);
      std::memcpy(&m->mpid, data + 34, 4);
      msg = m;
   };

   handlers[2] = [](const char *data, ITCHMessage &msg) noexcept
   {
      ITCHCancelMessage *m = nullptr;
      itch_pools_.get_pool<ITCHCancelMessage>().acquire(m);

      std::memcpy(&m->message_type, data, 1);
      std::memcpy(&m->timestamp, data + 1, 8);
      std::memcpy(&m->order_ref, data + 9, 8);
      std::memcpy(&m->cancelled_quantity, data + 17, 4);
      msg = m;
   };

   handlers[3] = [](const char *data, ITCHMessage &msg) noexcept
   {
      ITCHExecutedMessage *m = nullptr;
      itch_pools_.get_pool<ITCHExecutedMessage>().acquire(m);

      std::memcpy(&m->message_type, data, 1);
      std::memcpy(&m->timestamp, data + 1, 8);
      std::memcpy(&m->order_ref, data + 9, 8);
      std::memcpy(&m->executed_quantity, data + 17, 4);
      msg = m;
   };

   handlers[4] = [](const char *data, ITCHMessage &msg) noexcept
   {
      ITCHExecutedWithPriceMessage *m = nullptr;
      itch_pools_.get_pool<ITCHExecutedWithPriceMessage>().acquire(m);

      std::memcpy(&m->message_type, data, 1);
      std::memcpy(&m->timestamp, data + 1, 8);
      std::memcpy(&m->order_ref, data + 9, 8);
      std::memcpy(&m->executed_quantity, data + 17, 4);
      std::memcpy(&m->execution_price, data + 21, 4);
      msg = m;
   };

   handlers[5] = [](const char *data, ITCHMessage &msg) noexcept
   {
      ITCHDeleteMessage *m = nullptr;
      itch_pools_.get_pool<ITCHDeleteMessage>().acquire(m);

      std::memcpy(&m->message_type, data, 1);
      std::memcpy(&m->timestamp, data + 1, 8);
      std::memcpy(&m->order_ref, data + 9, 8);
      msg = m;
   };

   handlers[6] = [](const char *data, ITCHMessage &msg) noexcept
   {
      ITCHTradeMessage *m = nullptr;
      itch_pools_.get_pool<ITCHTradeMessage>().acquire(m);

      std::memcpy(&m->message_type, data, 1);
      std::memcpy(&m->timestamp, data + 1, 8);
      std::memcpy(&m->order_ref, data + 9, 8);
      std::memcpy(&m->stock, data + 17, 8);
      std::memcpy(&m->quantity, data + 25, 4);
      std::memcpy(&m->price, data + 29, 4);
      std::memcpy(&m->match_id, data + 33, 4);
      msg = m;
   };

   handlers[7] = [](const char *data, ITCHMessage &msg) noexcept
   {
      ITCHSystemEventMessage *m = nullptr;
      itch_pools_.get_pool<ITCHSystemEventMessage>().acquire(m);

      std::memcpy(&m->message_type, data, 1);
      std::memcpy(&m->timestamp, data + 1, 8);
      std::memcpy(&m->event_code, data + 9, 1);
      msg = m;
   };

   return handlers;
}

std::array<Parser_ITCH::MessageHandlerFunc, Parser_ITCH::MAX_MESSAGES> Parser_ITCH::MessageHandlers = makeMessageHandlersLookup();
