// ======================================================================================================
// Main
//
// PURPOSE:
// - Entry point of the trading system.
// - Initializes global logger, creates TradingEngine on the heap.
//
// THREAD SAFETY:
// - Main thread is single-threaded except for signal handling.
// - TradingEngine internally manages all flow threads; main thread only monitors
//   g_running atomic flag for shutdown.
//
// PERFORMANCE & DESIGN NOTES:
// - TradingEngine must be heap-allocated to avoid main thread stack overflow.
// - Main loop is idle-mostly, sleeping 100ms per iteration, ensuring low CPU usage
//   while flows operate independently.
// - Signals (SIGINT) are handled asynchronously; atomic flag ensures safe shutdown.
//
// DEVELOPER NOTES:
// - Logger::Init() must be called before creating or using any components.
// ======================================================================================================

#include "TradingEngine.h"
#include "config_utils.h"
#include "Logger.h"

#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>
#include <memory>

std::atomic<bool> g_running{true};

void signal_handler(int)
{
    g_running.store(false, std::memory_order_relaxed);
}

int main()
{
    configure_affinity(14);
    lock_memory();

    std::signal(SIGINT, signal_handler);
    
    Logger::Init();
    auto engine = std::make_unique<TradingEngine>();

    engine->start();

    while (g_running.load(std::memory_order_relaxed))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    engine->stop();
    Logger::Shutdown();

    return 0;
}
