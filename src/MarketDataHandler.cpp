/* #include "MarketDataHandler.h"

#include <iostream>
#include <sched.h>
#include <sys/mman.h>
#include <unistd.h>
#include <xmmintrin.h> // _mm_pause

MarketDataHandler::MarketDataHandler()
    : receiver_(receiver_to_parser_),
      parser_(receiver_to_parser_, parsed_to_store_),
      store_ram_(parsed_to_store_, store_to_strategy_, store_to_db_),
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
   configure_realtime();

   running_.store(true, std::memory_order_release);

   recv_thread_ = std::thread(&MarketDataHandler::recv_loop, this);
   parse_thread_ = std::thread(&MarketDataHandler::parse_loop, this);
   store_ram_thread_ = std::thread(&MarketDataHandler::store_ram_loop, this);
   store_db_thread_ = std::thread(&MarketDataHandler::store_db_loop, this);

   configure_affinity(0); // main thread
   configure_affinity(1); // receiver
   configure_affinity(2); // parser
   configure_affinity(3); // store_ram
   configure_affinity(4); // store_db
}

void MarketDataHandler::recv_loop() noexcept
{
   configure_affinity(1);

   while (__builtin_expect(running_.load(std::memory_order_acquire), 1))
   {
      receiver_.receive();
      _mm_pause(); // CPU relax
   }
}

void MarketDataHandler::parse_loop() noexcept
{
   configure_affinity(2);

   while (__builtin_expect(running_.load(std::memory_order_acquire), 1))
   {
      parser_.parse();
      _mm_pause();
   }
}

void MarketDataHandler::store_ram_loop() noexcept
{
   configure_affinity(3);

   while (__builtin_expect(running_.load(std::memory_order_acquire), 1))
   {
      store_ram_.store();
      _mm_pause();
   }
}

void MarketDataHandler::store_db_loop() noexcept
{
   configure_affinity(4);

   while (__builtin_expect(running_.load(std::memory_order_acquire), 1))
   {
      store_db_.store();
      _mm_pause();
   }
}

void MarketDataHandler::lock_memory()
{
   if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0)
   {
      perror("mlockall failed");
   }
}

void MarketDataHandler::configure_realtime()
{
   struct sched_param sched;
   sched.sched_priority = sched_get_priority_max(SCHED_FIFO);
   if (sched_setscheduler(0, SCHED_FIFO, &sched) != 0)
   {
      perror("sched_setscheduler failed");
   }
}

void MarketDataHandler::configure_affinity(int cpu_id)
{
   cpu_set_t cpuset;
   CPU_ZERO(&cpuset);
   CPU_SET(cpu_id, &cpuset);

   pthread_t thread = pthread_self();
   if (pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset) != 0)
   {
      perror("pthread_setaffinity_np failed");
   }
}
 */