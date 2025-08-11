#include "MarketDataHandler.h"
#include "common.h"
#include "config_utils.h"

#include <xmmintrin.h> // _mm_pause

MarketDataHandler::MarketDataHandler()
    : receiver_(receiver_to_parser_),
      parser_(receiver_to_parser_, parser_to_store_),
      store_ram_(parser_to_store_, store_to_strategy_, store_to_strategy_free_slot_, store_to_db_),
      store_db_(store_to_db_) {}

MarketDataHandler::~MarketDataHandler()
{
   running_.store(false, std::memory_order_release);

   if (recv_thread_.joinable())
      recv_thread_.join();
   if (parse_thread_.joinable())
      parse_thread_.join();
   if (store_ram_thread_.joinable())
      store_ram_thread_.join();
   if (store_db_thread_.joinable())
      store_db_thread_.join();
}

void MarketDataHandler::run()
{
   lock_memory();

   running_.store(true, std::memory_order_release);

   recv_thread_ = std::thread(&MarketDataHandler::recv_loop, this);
   parse_thread_ = std::thread(&MarketDataHandler::parse_loop, this);
   store_ram_thread_ = std::thread(&MarketDataHandler::store_ram_loop, this);
   store_db_thread_ = std::thread(&MarketDataHandler::store_db_loop, this);
}

void MarketDataHandler::recv_loop() noexcept
{
   configure_realtime(sched_get_priority_max(SCHED_FIFO));
   configure_affinity(0);

   while (LIKELY(running_.load(std::memory_order_acquire)))
   {
      std::cout << "receiver";
      receiver_.receive();
      _mm_pause(); // CPU relax
      std::cout << "receiver";
      break;
   }
}

void MarketDataHandler::parse_loop() noexcept
{
   configure_realtime(sched_get_priority_max(SCHED_FIFO));
   configure_affinity(1);

   while (LIKELY(running_.load(std::memory_order_acquire)))
   {
      parser_.dispatch();
      _mm_pause();
      break;
   }
}

void MarketDataHandler::store_ram_loop() noexcept
{
   configure_realtime(sched_get_priority_max(SCHED_FIFO));
   configure_affinity(2);

   while (LIKELY(running_.load(std::memory_order_acquire)))
   {
      store_ram_.store();
      _mm_pause();
      break;
   }
}

void MarketDataHandler::store_db_loop() noexcept
{
   configure_realtime(sched_get_priority_max(SCHED_FIFO) / 3);
   configure_affinity(3);

   while (LIKELY(running_.load(std::memory_order_acquire)))
   {
      store_db_.store();
      _mm_pause();
      break;
   }
}
