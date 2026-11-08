#include "RiskFlow.h"
#include "config_utils.h"
#include "FlowException.h"

#include <immintrin.h>

RiskFlow::RiskFlow(RiskEngine &risk) 
    : risk_(risk)
{
}

void RiskFlow::start()
{
    running_.store(true, std::memory_order_release);

    risk_check_thread_ = std::thread([this] {

        try 
        {
            configure_realtime(sched_get_priority_max(SCHED_FIFO)); 
            configure_affinity(12);         
            run_check(); 
        }
        catch (...) 
        {
            thread_failure.report(std::current_exception());
        }   
    });

    risk_update_thread_ = std::thread([this] {
        
        try 
        {
            configure_realtime(sched_get_priority_max(SCHED_FIFO)); 
            configure_affinity(10);       
            run_update(); 
        }
        catch (...) 
        {
            thread_failure.report(std::current_exception());
        }   
   });
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