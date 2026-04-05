#include "RiskFlow.h"
#include "config_utils.h"

#include <immintrin.h>

RiskFlow::RiskFlow(RiskEngine &risk) noexcept
    : risk_(risk)
{
}

void RiskFlow::start() noexcept
{
    running_.store(true, std::memory_order_release);

    risk_check_thread_ = std::thread([this]
                                  {
   
    configure_realtime(sched_get_priority_max(SCHED_FIFO)); 
    configure_affinity(8);         

    run_check(); });

    risk_update_thread_ = std::thread([this]
                                     {
   
    configure_realtime(sched_get_priority_max(SCHED_FIFO)); 
    configure_affinity(10);       

    run_update(); });
}

void RiskFlow::run_check() noexcept
{
    while (running_.load(std::memory_order_acquire))
    {
        if(!risk_.check_risk())
            _mm_pause();
    }
}

void RiskFlow::run_update() noexcept
{
    while (running_.load(std::memory_order_acquire))
    {
        if(!risk_.update_risk())
            _mm_pause();
    }
}

void RiskFlow::stop() noexcept
{
    running_.store(false, std::memory_order_release);

    if (risk_check_thread_.joinable())
        risk_check_thread_.join();

    if (risk_update_thread_.joinable())
        risk_update_thread_.join();
}