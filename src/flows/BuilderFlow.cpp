#include "BuilderFlow.h"
#include "config_utils.h"

#include <immintrin.h>

BuilderFlow::BuilderFlow(Builder_Dispatch &builder_dispatch) noexcept
    : builder_dispatch_(builder_dispatch)
{
}

void BuilderFlow::start() noexcept
{
    running_.store(true, std::memory_order_release);

    builder_thread_ = std::thread([this]
                                  {
   
    configure_realtime(sched_get_priority_max(SCHED_FIFO)); 
    configure_affinity(8);       
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