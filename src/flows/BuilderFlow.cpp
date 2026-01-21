#include "flows/BuilderFlow.h"
#include "config_utils.h"

BuilderFlow::BuilderFlow(Builder_Dispatch &builder_dispatch) noexcept
    : builder_dispatch_(builder_dispatch)
{
}

void BuilderFlow::start() noexcept
{
    running_.store(true, std::memory_order_release);

    builder_thread_ = std::thread([this]
                                  {
   
    lock_memory();                 // mlockall
    configure_realtime(sched_get_priority_max(SCHED_FIFO)); // RT priority
    configure_affinity(12);         // CPU core (örnek)

    run(); });
}

void BuilderFlow::run() noexcept
{
    while (running_.load(std::memory_order_acquire))
    {
        if(!builder_dispatch_.dispatch())
            _mm_pause();
    }
}

void BuilderFlow::stop() noexcept
{
    running_.store(false, std::memory_order_release);

    if (builder_thread_.joinable())
        builder_thread_.join();
}