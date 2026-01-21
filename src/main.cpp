#include "TradingEngine.h"

#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

std::atomic<bool> g_running{true};


void signal_handler(int)
{
    g_running.store(false, std::memory_order_relaxed);
}

int main()
{
    std::signal(SIGINT, signal_handler);
   
    TradingEngine engine;
   
    engine.start();

    while (g_running.load(std::memory_order_relaxed))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    engine.stop();

    return 0;
}
