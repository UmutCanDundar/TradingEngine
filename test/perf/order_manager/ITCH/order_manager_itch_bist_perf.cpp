#include "OrderManager.h"
#include "MarketBook.h"
#include "HashTables.h"
#include "Order.h"
#include "dataset_order_manager.h"
#include "perf_utils.h"

#include <thread>
#include <atomic>
#include <array>
#include <memory>

int main()
{
    pin_to_cpu(6);

    std::unique_ptr<HashTables>   hashtables;
    std::unique_ptr<MarketBook>   marketbook;
    std::unique_ptr<OrderManager> order_manager;

    spscMessageQueue_t parser_to_store;
    spscOrderQueue_t   store_to_strategy;
    spscOrderQueue_t   store_to_strategy_free_slot;
    spscOrderQueue_t   store_to_risk;
    spscDbQueue_t      store_to_db;

    std::atomic<bool> stop{false};
    std::atomic<bool> stop2{false};
    std::atomic<bool> stop3{false};

    std::thread consumer;
    std::thread consumer2;
    std::thread consumer3;

    std::array<MessageWithVenue<MessageTypes_t>, 4> ord_manager_traffic;

    hashtables    = std::make_unique<HashTables>();
    marketbook    = std::make_unique<MarketBook>(*hashtables);
    order_manager = std::make_unique<OrderManager>(
                        parser_to_store,
                        store_to_strategy,
                        store_to_strategy_free_slot,
                        store_to_risk,
                        store_to_db,
                        *marketbook,
                        *hashtables
    );

    ord_manager_traffic = []()
    {
        std::array<MessageWithVenue<MessageTypes_t>, 4> arr{};
        arr[0] = MessageWithVenue<MessageTypes_t>{BIST::ITCHMessage{&test_data_ordMngr::bist_dir},    Venue::BIST};
        arr[1] = MessageWithVenue<MessageTypes_t>{BIST::ITCHMessage{&test_data_ordMngr::bist_add},    Venue::BIST};
        arr[2] = MessageWithVenue<MessageTypes_t>{BIST::ITCHMessage{&test_data_ordMngr::bist_exec},   Venue::BIST};
        arr[3] = MessageWithVenue<MessageTypes_t>{BIST::ITCHMessage{&test_data_ordMngr::bist_cancel}, Venue::BIST};
        return arr;
    }();

    parser_to_store.push(ord_manager_traffic[0]);
    order_manager->store();

    consumer = std::thread([&]
    {
        pin_to_cpu(0);
        Order* order;
        while (!stop.load(std::memory_order_acquire))
            if (!store_to_risk.pop(order)) _mm_pause();
    });

    consumer2 = std::thread([&]
    {
        pin_to_cpu(2);
        Order* order;
        while (!stop2.load(std::memory_order_acquire))
            if (!store_to_strategy.pop(order)) _mm_pause();
    });

    consumer3 = std::thread([&]
    {
        pin_to_cpu(4);
        DbData_t DbData;
        while (!stop3.load(std::memory_order_acquire))
            if (!store_to_db.pop(DbData)) _mm_pause();
    });

    auto run = [&]()
    {
        for (size_t j = 1; j < 4; j++)
            parser_to_store.push(ord_manager_traffic[j]);

        asm volatile("" ::: "memory");

        bool processing = true;
        while (processing)
            processing = order_manager->store();
    };

    constexpr int WARMUP = 10'000;
    for (int i = 0; i < WARMUP; i++)
        run();

    constexpr int N = 5'000'000;
    for (int i = 0; i < N; i++)
        run();

    stop.store(true, std::memory_order_release);
    stop2.store(true, std::memory_order_release);
    stop3.store(true, std::memory_order_release);
    consumer.join();
    consumer2.join();
    consumer3.join();

    order_manager.reset();
    marketbook.reset();
    hashtables.reset();

    return 0;
}