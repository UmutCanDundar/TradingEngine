#pragma once

#include "Builder_Dispatch.h"
#include <atomic>
#include <thread>

class BuilderFlow
{
public:
    BuilderFlow(Builder_Dispatch &Builder_dispatch) noexcept;

    void start() noexcept;
    void stop() noexcept;

private:
    void run() noexcept;

private:
    Builder_Dispatch &builder_dispatch_;
    std::atomic<bool> running_{false};
    std::thread builder_thread_;
};