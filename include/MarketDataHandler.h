#pragma once

#include "Receiver.h"
#include "Parser_Dispatch.h"
#include "Store_RAM.h"
#include "Store_DB.h"

#include <atomic>

class MarketDataHandler final
{
public:
    MarketDataHandler();
    ~MarketDataHandler();

    void run(); // sistemi başlatır

private:
    // Lock-free Queue'lar
    spscPacketQueue_t receiver_to_parser_;
    spscMessageQueue_t parser_to_store_;
    spscOrderQueue_t store_to_strategy_;
    spscDbQueue_t store_to_db_;

    // Komponentler
    Receiver receiver_;
    Parser_Dispatch parser_;
    Store_RAM store_ram_;
    Store_DB store_db_;

    // Threadler
    std::thread recv_thread_;
    std::thread parse_thread_;
    std::thread store_ram_thread_;
    std::thread store_db_thread_;

    // Çalışma bayrağı
    std::atomic<bool> running_;

    // Dahili thread loop'lar
    void recv_loop() noexcept;
    void parse_loop() noexcept;
    void store_ram_loop() noexcept;
    void store_db_loop() noexcept;
};
