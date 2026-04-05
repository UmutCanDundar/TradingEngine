#include "NetworkFlow.h"
#include "config_utils.h"

#include <immintrin.h>

NetworkFlow::NetworkFlow(NetworkIO &netIO) noexcept
    : netIO_(netIO)
{
}

void NetworkFlow::start() noexcept
{
    running_.store(true, std::memory_order_release);

    network_thread_ = std::thread([this] {
   
    configure_realtime(sched_get_priority_max(SCHED_FIFO)); 
    configure_affinity(0);         

    run(); });
}

void NetworkFlow::run() noexcept
{
    while (running_.load(std::memory_order_acquire))
    {
        netIO_.recv_send();
    }
}

void NetworkFlow::stop() noexcept
{
    running_.store(false, std::memory_order_release);

    if (network_thread_.joinable())
        network_thread_.join();
}
