#include "OrderFlow.h"
#include "config_utils.h"
#include "FlowException.h"

#include <immintrin.h>

OrderFlow::OrderFlow(OrderManager& ord_mngr, ClickhouseWriter& ch_writer)
    : ord_mngr_(ord_mngr), ch_writer_(ch_writer)
{
}

void OrderFlow::start() 
{
    running_.store(true, std::memory_order_release);

    ord_mngr_thread_ = std::thread([this] {
       
        try 
        {
            configure_realtime(sched_get_priority_max(SCHED_FIFO)); 
            configure_affinity(4);       
            run_order_manager();
        }
        catch (...) 
        {
            thread_failure.report(std::current_exception());
        }     
    });

    ch_writer_thread_ = std::thread([this] {
    
        try 
        {
            configure_realtime(sched_get_priority_max(SCHED_FIFO)); 
            configure_affinity(13);        
            run_clickhouse_writer();
        }
        catch (...) 
        {
            thread_failure.report(std::current_exception());
        }    
    });
}

void OrderFlow::run_order_manager() noexcept
{
    while (running_.load(std::memory_order_acquire))
    {
        if(!ord_mngr_.store())
            _mm_pause();
    }
}

void OrderFlow::run_clickhouse_writer() noexcept
{
    while (running_.load(std::memory_order_acquire))
    {
        if (!ch_writer_.store())
            _mm_pause();
    }
}

void OrderFlow::stop() noexcept
{
    running_.store(false, std::memory_order_release);

    if (ord_mngr_thread_.joinable())
        ord_mngr_thread_.join();

    if (ch_writer_thread_.joinable())
        ch_writer_thread_.join();
}