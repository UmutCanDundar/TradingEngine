#include "flows/OrderFlow.h"
#include "config_utils.h"

OrderFlow::OrderFlow(Store_RAM& store_ram, Store_DB& store_db) noexcept
    : store_ram_(store_ram), store_db_(store_db)
{
}

void OrderFlow::start() noexcept
{
    running_.store(true, std::memory_order_release);

    store_ram_thread_ = std::thread([this]
                                     {
   
    lock_memory();                 // mlockall
    configure_realtime(sched_get_priority_max(SCHED_FIFO)); // RT priority
    configure_affinity(4);         // CPU core (örnek)

    run_ram(); });

    store_db_thread_ = std::thread([this]
                                      {
   
    lock_memory();                 // mlockall
    configure_realtime(sched_get_priority_max(SCHED_FIFO) / 3); // RT priority
    configure_affinity(15);         // CPU core (örnek)

    run_db(); });
}

void OrderFlow::run_ram() noexcept
{
    while (running_.load(std::memory_order_acquire))
    {
        if(!store_ram_.store())
            _mm_pause();
    }
}

void OrderFlow::run_db() noexcept
{
    while (running_.load(std::memory_order_acquire))
    {
        if (!store_db_.store())
            _mm_pause();
    }
}

void OrderFlow::stop() noexcept
{
    running_.store(false, std::memory_order_release);

    if (store_ram_thread_.joinable())
        store_ram_thread_.join();

    if (store_db_thread_.joinable())
        store_db_thread_.join();
}