#pragma once

#include "NetworkIO.h"
#include <atomic>
#include <thread>

class NetworkFlow
{
private:
    NetworkIO &netIO_;
    std::atomic<bool> running_{false};
    std::thread network_thread_;

public:
    NetworkFlow(NetworkIO &netIO) noexcept;

    void start() noexcept;
    void stop() noexcept;

private:
    void run() noexcept;

};
