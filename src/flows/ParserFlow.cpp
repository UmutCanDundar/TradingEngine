#include "ParserFlow.h"
#include "config_utils.h"
#include "FlowException.h"

#include <immintrin.h>

ParserFlow::ParserFlow(Parser_Dispatch &parser_dispatch) 
    : parser_dispatch_(parser_dispatch)
{
}

void ParserFlow::start() 
{
    running_.store(true, std::memory_order_release);

    parser_thread_ = std::thread([this] {
   
        try 
        {
            configure_realtime(sched_get_priority_max(SCHED_FIFO)); 
            configure_affinity(2);       
            run();
        }
        catch (...) 
        {
            thread_failure.report(std::current_exception());
        }   

    });
}

void ParserFlow::run() noexcept
{
    while (running_.load(std::memory_order_acquire))
    {
        if(!parser_dispatch_.dispatch())
            _mm_pause();
    }
}

void ParserFlow::stop() noexcept
{
    running_.store(false, std::memory_order_release);

    if (parser_thread_.joinable())
        parser_thread_.join();
}