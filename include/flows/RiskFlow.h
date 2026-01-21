#pragma once

#include "RiskEngine.h"
#include <atomic>
#include <thread>

class RiskFlow
{
public:
    RiskFlow(RiskEngine &risk) noexcept;

    void start() noexcept;
    void stop() noexcept;

private:
    void run_check() noexcept;
    void run_update() noexcept;

private:
    RiskEngine &risk_;
    std::atomic<bool> running_{false};
    std::thread risk_check_thread_;
    std::thread risk_update_thread_;

};