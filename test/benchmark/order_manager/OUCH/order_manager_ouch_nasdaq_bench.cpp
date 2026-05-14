#include <benchmark/benchmark.h>

#include "OrderManager.h"
#include "MarketBook.h"
#include "HashTables.h"
#include "Order.h"
#include "dataset_order_manager.h"
#include "bench_utils.h"

#include <thread>
#include <atomic>
#include <vector>
#include <array>
#include <memory>
#include <chrono>

class BM_OrderManager : public benchmark::Fixture
{
public:
    std::unique_ptr<HashTables> hashtables;
    std::unique_ptr<MarketBook> marketbook;
    std::unique_ptr<OrderManager> order_manager;

    spscMessageQueue_t parser_to_store;
    spscOrderQueue_t store_to_strategy;
    spscOrderQueue_t store_to_strategy_free_slot;
    spscOrderQueue_t store_to_risk;
    spscDbQueue_t store_to_db;

    std::atomic<bool> stop{false};
    std::thread consumer;

    std::atomic<bool> stop2{false};
    std::thread consumer2;

    std::atomic<bool> stop3{false};
    std::thread consumer3;

    std::array<MessageWithVenue<MessageTypes_t>, 4> ord_manager_traffic;

public:
    
    void SetUp(const ::benchmark::State& st) 
    {
        stop.store(false, std::memory_order_release);
        stop2.store(false, std::memory_order_release);
        stop3.store(false, std::memory_order_release);
        
        Order* order;
        while(store_to_strategy_free_slot.pop(order)) {}
    
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

            // 0 — NASDAQ ITCH StockDirectory (AAPL)
            arr[0] = MessageWithVenue<MessageTypes_t>{NASDAQ::ITCHMessage{&test_data_ordMngr::nasdaq_dir}, Venue::NASDAQ};

            // 2 — NASDAQ OUCH OrderAccepted (AAPL, user_ref=99)
            arr[1] = MessageWithVenue<MessageTypes_t>{NASDAQ::OUCHMessage{&test_data_ordMngr::nasdaq_acc}, Venue::NASDAQ};   //////////////////

            // 3 — NASDAQ OUCH OrderExecuted (AAPL, 600 executed)
            arr[2] = MessageWithVenue<MessageTypes_t>{NASDAQ::OUCHMessage{&test_data_ordMngr::nasdaq_execO}, Venue::NASDAQ};   //////////////

            // 4 — NASDAQ OUCH OrderCancelled (AAPL)
            arr[3] = MessageWithVenue<MessageTypes_t>{NASDAQ::OUCHMessage{&test_data_ordMngr::nasdaq_canO}, Venue::NASDAQ};  //////////////////

            return arr;
        }();
        
        for(size_t i = 0; i < 1; i++)
        {
            parser_to_store.push(ord_manager_traffic[i]);
            order_manager->store();
        }

        consumer = std::thread([&]
        {
            pin_to_cpu(0);        

            Order* order; 
            
            while(!stop.load(std::memory_order_acquire))
            {
                if(!store_to_risk.pop(order)) 
                {
                    _mm_pause();  
                }
            }
        });

        consumer2 = std::thread([&]
        {
            pin_to_cpu(4);        

            Order* order; 
            
            while(!stop2.load(std::memory_order_acquire))
            {
                if(!store_to_strategy.pop(order)) 
                {
                    _mm_pause();  
                }
            }
        });

        consumer3 = std::thread([&]
        {
            pin_to_cpu(6);        

            Order* order; 
            
            DbData_t DbData; 

            while(!stop3.load(std::memory_order_acquire))
            {
                if(!store_to_db.pop(DbData)) 
                {
                    _mm_pause();  
                }
            }
        });
    }

    void refresh_awaiting_map_per_ite()
    {
        // NASDAQ OUCH order
        {
            Order* order = nullptr;
            store_to_strategy_free_slot.pop(order);

            order->price              = test_data_ordMngr::nasdaq_acc.price;
            order->quantity           = test_data_ordMngr::nasdaq_acc.quantity;
            order->side               = Side::Buy;
            order->venue              = Venue::NASDAQ;
            order->isOurOrder         = true;
            order->protocol           = Protocol::OUCH;
            order->user_ref_num       = test_data_ordMngr::nasdaq_acc.user_ref_num;
            order->symbol             = {"AAPL"};
            order->client_order_id    = absl::Hash<std::string_view>{}(test_data_ordMngr::nasdaq_acc.cl_ord_id);
            std::strncpy(order->client_order_token.data(), test_data_ordMngr::nasdaq_acc.cl_ord_id, sizeof(test_data_ordMngr::nasdaq_acc.cl_ord_id));

            order_manager->add_awaitingAck_order(*order);
        }
    }

    void TearDown(const ::benchmark::State& st) 
    {
        stop.store(true, std::memory_order_release);
        stop2.store(true, std::memory_order_release);
        stop3.store(true, std::memory_order_release);
        consumer.join();
        consumer2.join();
        consumer3.join();

        order_manager.reset();
        marketbook.reset();
        hashtables.reset();

    }
};

BENCHMARK_DEFINE_F(BM_OrderManager, OuchNasdaq)(benchmark::State& state)
{
    pin_to_cpu(2);

    std::vector<uint64_t> latencies;
    latencies.reserve(100000);

    for(auto _ : state)
    {
        for(size_t i = 1; i < 4; i++)
            parser_to_store.push(ord_manager_traffic[i]);

        bool processing = true;
        refresh_awaiting_map_per_ite();

        asm volatile("" ::: "memory");
        auto wall_start = std::chrono::high_resolution_clock::now();
        uint64_t start = rdtsc_start();

        while (processing)
            processing = order_manager->store();

        uint64_t end = rdtsc_end();
        auto wall_end = std::chrono::high_resolution_clock::now();
        
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(wall_end - wall_start).count();
        state.SetIterationTime(elapsed * 1e-9);
        
        benchmark::ClobberMemory();
        latencies.push_back(end - start);

        for(auto& [key, ord] : order_manager->our_orders_)
            store_to_strategy_free_slot.push(ord);
        order_manager->our_orders_.clear();
    
    }
   
    if (latencies.empty()) return;

    std::sort(latencies.begin(), latencies.end());

    auto percentile = [&](double p)
    {
        size_t idx = std::min(
            static_cast<size_t>(p * latencies.size()),
            latencies.size() - 1
        );
        return latencies[idx];
    };

    state.counters["p050_cycles"] = percentile(0.50);
    state.counters["p095_cycles"] = percentile(0.95);
    state.counters["p099_cycles"] = percentile(0.99);
    state.counters["p999_cycles"] = percentile(0.999);
    state.counters["|max_cycles|"] = latencies.back();
}

BENCHMARK_REGISTER_F(BM_OrderManager, OuchNasdaq)->UseManualTime();

   
  
    

