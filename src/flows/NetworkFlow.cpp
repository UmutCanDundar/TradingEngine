#include "flows/NetworkFlow.h"
#include "config_utils.h"

NetworkFlow::NetworkFlow(NetworkIO &netIO) noexcept
    : netIO_(netIO)
{
}

void NetworkFlow::start() noexcept
{
    running_.store(true, std::memory_order_release);

    network_thread_ = std::thread([this] {
   
    lock_memory();                 // mlockall
    configure_realtime(sched_get_priority_max(SCHED_FIFO)); // RT priority
    configure_affinity(0);         // CPU core (örnek)

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
