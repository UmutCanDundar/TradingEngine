#pragma once

#include "Store_RAM.h"
#include "Store_DB.h"
#include <atomic>
#include <thread>

class OrderFlow
{
public:
    OrderFlow(Store_RAM &store_ram, Store_DB& store_db) noexcept;

    void start() noexcept;
    void stop() noexcept;

private:
    void run_ram() noexcept;
    void run_db() noexcept;

private:
    Store_RAM &store_ram_;
    Store_DB &store_db_;
    std::atomic<bool> running_{false};
    std::thread store_ram_thread_;
    std::thread store_db_thread_;
};