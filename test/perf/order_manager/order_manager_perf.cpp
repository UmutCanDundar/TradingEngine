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

    std::array<MessageWithVenue<MessageTypes_t>, 17> ord_manager_traffic;

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
        std::array<MessageWithVenue<MessageTypes_t>, 17> arr{};
        arr[0]  = MessageWithVenue<MessageTypes_t>{NASDAQ::ITCHMessage{&test_data_ordMngr::nasdaq_dir},    Venue::NASDAQ};
        arr[1]  = MessageWithVenue<MessageTypes_t>{BIST::ITCHMessage{&test_data_ordMngr::bist_dir},        Venue::BIST};
        arr[2]  = MessageWithVenue<MessageTypes_t>{&test_data_ordMngr::fix_new,                            Venue::BIST};
        arr[3]  = MessageWithVenue<MessageTypes_t>{BIST::OUCHMessage{&test_data_ordMngr::bist_acc},        Venue::BIST};
        arr[4]  = MessageWithVenue<MessageTypes_t>{BIST::ITCHMessage{&test_data_ordMngr::bist_add},        Venue::BIST};
        arr[5]  = MessageWithVenue<MessageTypes_t>{NASDAQ::OUCHMessage{&test_data_ordMngr::nasdaq_acc},    Venue::NASDAQ};
        arr[6]  = MessageWithVenue<MessageTypes_t>{NASDAQ::ITCHMessage{&test_data_ordMngr::nasdaq_add},    Venue::NASDAQ};
        arr[7]  = MessageWithVenue<MessageTypes_t>{&test_data_ordMngr::fix_partial,                        Venue::BIST};
        arr[8]  = MessageWithVenue<MessageTypes_t>{BIST::ITCHMessage{&test_data_ordMngr::bist_exec},       Venue::BIST};
        arr[9]  = MessageWithVenue<MessageTypes_t>{BIST::OUCHMessage{&test_data_ordMngr::bist_execO},      Venue::BIST};
        arr[10] = MessageWithVenue<MessageTypes_t>{NASDAQ::ITCHMessage{&test_data_ordMngr::nasdaq_exec},   Venue::NASDAQ};
        arr[11] = MessageWithVenue<MessageTypes_t>{NASDAQ::OUCHMessage{&test_data_ordMngr::nasdaq_execO},  Venue::NASDAQ};
        arr[12] = MessageWithVenue<MessageTypes_t>{BIST::ITCHMessage{&test_data_ordMngr::bist_cancel},     Venue::BIST};
        arr[13] = MessageWithVenue<MessageTypes_t>{BIST::OUCHMessage{&test_data_ordMngr::bist_canO},       Venue::BIST};
        arr[14] = MessageWithVenue<MessageTypes_t>{NASDAQ::ITCHMessage{&test_data_ordMngr::nasdaq_delete}, Venue::NASDAQ};
        arr[15] = MessageWithVenue<MessageTypes_t>{NASDAQ::OUCHMessage{&test_data_ordMngr::nasdaq_canO},   Venue::NASDAQ};
        arr[16] = MessageWithVenue<MessageTypes_t>{&test_data_ordMngr::fix_cancel,                         Venue::BIST};
        return arr;
    }();

    for (size_t i = 0; i < 2; i++)
    {
        parser_to_store.push(ord_manager_traffic[i]);
        order_manager->store();
    }

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

    auto refresh_awaiting_map = [&]()
    {
        {
            Order* order = nullptr;
            store_to_strategy_free_slot.pop(order);

            order->price           = test_data_ordMngr::fix_new.price;
            order->quantity        = test_data_ordMngr::fix_new.quantity;
            order->side            = Side::Buy;
            order->venue           = Venue::BIST;
            order->isOurOrder      = true;
            order->protocol        = Protocol::FIX;
            order->instrument_id   = 3;
            order->order_type      = static_cast<OrderType>(test_data_ordMngr::fix_new.ord_type);
            order->symbol          = {"GARAN"};
            std::strncpy(order->client_order_token.data(), test_data_ordMngr::fix_new.cl_ord_id, ORDER_TOKEN_SIZE);
            order->client_order_id = absl::Hash<std::string_view>{}(test_data_ordMngr::fix_new.cl_ord_id);

            order_manager->add_awaitingAck_order(*order);
        }

        {
            Order* order = nullptr;
            store_to_strategy_free_slot.pop(order);

            order->price           = test_data_ordMngr::bist_acc.price;
            order->quantity        = test_data_ordMngr::bist_acc.quantity;
            order->side            = Side::Buy;
            order->venue           = Venue::BIST;
            order->isOurOrder      = true;
            order->protocol        = Protocol::OUCH;
            order->instrument_id   = test_data_ordMngr::bist_acc.order_book_id;
            order->symbol          = {"GARAN"};
            std::strncpy(order->client_order_token.data(), test_data_ordMngr::bist_acc.order_token, sizeof(test_data_ordMngr::bist_acc.order_token));
            order->client_order_id = absl::Hash<std::string_view>{}(std::string_view{test_data_ordMngr::bist_acc.order_token, 14});

            order_manager->add_awaitingAck_order(*order);
        }

        {
            Order* order = nullptr;
            store_to_strategy_free_slot.pop(order);

            order->price           = test_data_ordMngr::nasdaq_acc.price;
            order->quantity        = test_data_ordMngr::nasdaq_acc.quantity;
            order->side            = Side::Buy;
            order->venue           = Venue::NASDAQ;
            order->isOurOrder      = true;
            order->protocol        = Protocol::OUCH;
            order->user_ref_num    = test_data_ordMngr::nasdaq_acc.user_ref_num;
            order->symbol          = {"AAPL"};
            order->client_order_id = absl::Hash<std::string_view>{}(std::string_view{test_data_ordMngr::nasdaq_acc.cl_ord_id, 14});
            std::strncpy(order->client_order_token.data(), test_data_ordMngr::nasdaq_acc.cl_ord_id, sizeof(test_data_ordMngr::nasdaq_acc.cl_ord_id));

            order_manager->add_awaitingAck_order(*order);
        }
    };

    auto run = [&]()
    {
        for (size_t j = 2; j < 17; j++)
            parser_to_store.push(ord_manager_traffic[j]);

        refresh_awaiting_map(); 
        asm volatile("" ::: "memory");

        bool processing = true;
        while (processing)
            processing = order_manager->store();

        for(auto& [key, ord] : order_manager->our_orders_)
            store_to_strategy_free_slot.push(ord);
        order_manager->our_orders_.clear();
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