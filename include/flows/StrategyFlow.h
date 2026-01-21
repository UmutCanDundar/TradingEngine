// #pragma once

// #include "Strategy.h"
// #include <atomic>
// #include <thread>

// class StrategyFlow
// {
// public:
//     StrategyFlow(Strategy &strategy) noexcept;

//     void start() noexcept;
//     void stop() noexcept;

// private:
//     void run() noexcept;

// private:
//     Strategy &strategy_;
//     std::atomic<bool> running_{false};
//     std::thread strategy_thread_;
// };