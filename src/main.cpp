// ======================================================================================================
// Main
//
// PURPOSE:
// - Entry point of the trading system.
// - Initializes global logger, creates TradingEngine on the heap, handles shutdown signals.
//
// SIGNAL DESIGN:
// - SIGINT / SIGTERM / SIGHUP  -> graceful shutdown request (async handler only sets atomic flags).
// - SIGPIPE                    -> ignored (a peer closing a socket must never kill the process).
// - Shutdown signals are BLOCKED before any other thread exists, so Logger and all flow threads
//   inherit the blocked mask and never receive them.
//   Only the main thread unblocks them after the engine has been started.
// - Second signal falls back to the default action (SA_RESETHAND), so a stuck shutdown can be
//   killed with a second Ctrl+C.
// ======================================================================================================

#include "TradingEngine.h"
#include "config_utils.h"
#include "Logger.h"
#include "FlowException.h"

#include <csignal>
#include <pthread.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <memory>
#include <thread>

namespace 
{
    std::atomic<bool> g_running{true};
    std::atomic<int> g_signal{0};

    static_assert(std::atomic<bool>::is_always_lock_free, "handler flag must be lock-free");
    static_assert(std::atomic<int>::is_always_lock_free, "handler flag must be lock-free");

    void shutdown_handler(int signo) noexcept
    {
        g_signal.store(signo, std::memory_order_relaxed);
        g_running.store(false, std::memory_order_relaxed);
    }

    sigset_t shutdown_signals() noexcept
    {
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, SIGINT);
        sigaddset(&set, SIGTERM);
        sigaddset(&set, SIGHUP);
        return set;
    }

    bool install_signal_handlers() noexcept
    {
        struct sigaction sa{};
        sa.sa_handler = shutdown_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESETHAND; 

        if (sigaction(SIGINT, &sa, nullptr) != 0 ||
            sigaction(SIGTERM, &sa, nullptr) != 0 ||
            sigaction(SIGHUP, &sa, nullptr) != 0)
        {
            return false;
        }

        struct sigaction ign{};
        ign.sa_handler = SIG_IGN;
        sigemptyset(&ign.sa_mask);
        return sigaction(SIGPIPE, &ign, nullptr) == 0;
    }
} 

int main()
{
    // SIGNAL HANDLING
    const sigset_t shutdown_set = shutdown_signals();
    
    if (pthread_sigmask(SIG_BLOCK, &shutdown_set, nullptr) != 0)
    {
        std::fputs("pthread_sigmask(SIG_BLOCK) failed\n", stderr);
        return 1;
    }

    if (!install_signal_handlers())
    {
        std::fputs("sigaction failed\n", stderr);
        return 1;
    } 

    // ENGINE START
    Logger::Init();

    configure_affinity(14);
    lock_memory();

    std::unique_ptr<TradingEngine> engine;

    try
    {
        engine = std::make_unique<TradingEngine>();
        engine->start();
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("An exception was catched during TradingEngine initialization: {}", e.what());
        engine.reset(); 
        Logger::Shutdown();
        return 1;
    }
    
    pthread_sigmask(SIG_UNBLOCK, &shutdown_set, nullptr);

    // ENGINE RUNNING
    while (g_running.load(std::memory_order_relaxed) && !thread_failure.has_failed())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // ENGINE STOP (SIGNAL OR FAILURE)
    int exitCODE = 0;
    if (thread_failure.has_failed())
    {
        exitCODE = 1;
        try 
        { 
            std::rethrow_exception(thread_failure.error); 
        }
        catch (const std::exception &e) 
        { 
            LOG_ERROR("Flow thread failed: {}", e.what()); 
        }
        
    }
    else if (const int sig = g_signal.load(std::memory_order_relaxed); sig != 0)
    {
        LOG_INFO("Shutdown requested by signal {} ({})", sig, strsignal(sig));
    }

    engine->stop();
    engine.reset();      
    Logger::Shutdown();

    return exitCODE;
}


