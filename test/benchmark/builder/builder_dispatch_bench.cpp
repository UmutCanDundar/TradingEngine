#include <benchmark/benchmark.h>

#include "MarketBook.h"
#include "HashTables.h"
#include "Order.h"
#include "dataset_builder.h"
#include "SessionManager.h"
#include "LoginController.h"
#include "SoupBinTcp.h"
#include "Builder_FIX.h"
#include "Parser_FIX.h"
#include "NetworkPackets.h"
#include "FIXMessage.h"
#include "Builder_Dispatch.h"
#include "OrderManager.h"
#include "bench_utils.h"

#include <thread>
#include <atomic>
#include <vector>
#include <array>
#include <memory>
#include <chrono>

class BM_Builder : public benchmark::Fixture
{
public:
    std::unique_ptr<InPacketPoolManager> inPkt_pool;
    std::unique_ptr<HashTables> hashtables;
    std::unique_ptr<MarketBook> marketbook;
    std::unique_ptr<SessionManager> sess_mngr;
    std::unique_ptr<SoupBinTcp> sbt;
    std::unique_ptr<Builder_FIX> builder_fix;
    std::unique_ptr<LoginController> login;
    std::unique_ptr<Parser_FIX> parser_fix;
    std::unique_ptr<OrderManager> order_manager;
    std::unique_ptr<Builder_Dispatch> builder_dispatch;

    spscFIXInSessionQueue_t parser_to_fixbuilder_in;
    spscFIXOutSessionQueue_t parser_to_fixbuilder_out;
    spscInPacketQueue_t builder_to_sender;
    spscOrderQueue_t risk_to_builder;
    
    spscOrderQueue_t store_to_strategy_free_slot;
    spscOrderQueue_t store_to_risk;
    spscOrderQueue_t store_to_strategy;
    spscMessageQueue_t parser_to_store;
    spscDbQueue_t store_to_db;

    std::atomic<bool> stop{false};
    std::thread consumer;

    std::array<Order*, 3> orders;

    uint8_t sess_index;
    
public:
    
    void SetUp(const ::benchmark::State& st) 
    {
        stop.store(false, std::memory_order_release);
        
        inPkt_pool    = std::make_unique<InPacketPoolManager>();
        hashtables    = std::make_unique<HashTables>();
        marketbook    = std::make_unique<MarketBook>(*hashtables);
        sess_mngr     = std::make_unique<SessionManager>();
        sbt           = std::make_unique<SoupBinTcp>(*sess_mngr);
        builder_fix   = std::make_unique<Builder_FIX>(*sess_mngr);
        login         = std::make_unique<LoginController>(*sbt, *builder_fix, *sess_mngr);
        parser_fix    = std::make_unique<Parser_FIX>(parser_to_fixbuilder_in);
        order_manager = std::make_unique<OrderManager>(
                                            parser_to_store,
                                            store_to_strategy,
                                            store_to_strategy_free_slot,
                                            store_to_risk,
                                            store_to_db,
                                            *marketbook,
                                            *hashtables        
        );

        builder_dispatch = std::make_unique<Builder_Dispatch>(
                                            builder_to_sender,
                                            risk_to_builder,
                                            parser_to_fixbuilder_out,
                                            parser_to_fixbuilder_in,
                                            *sess_mngr,
                                            *sbt,
                                            *login,
                                            *inPkt_pool,
                                            *builder_fix,
                                            *order_manager,
                                            *parser_fix        
        );
        
        sess_index = sess_mngr->getSessionIndex(Venue::BIST, Protocol::FIX);

        orders = {
            test_data_builder::fix_new_order, 
            test_data_builder::ouch_new_order,
            test_data_builder::NQouch_new_order
        };

        consumer = std::thread([&]
        {
            pin_to_cpu(0);        

            InPacket* inPkt; 
            
            while(!stop.load(std::memory_order_acquire))
            {
                if(!builder_to_sender.pop(inPkt)) 
                {
                    _mm_pause();  
                }
            }
        });
    }

    void TearDown(const ::benchmark::State& st) 
    {
        stop.store(true, std::memory_order_release);
        consumer.join();
        
        builder_dispatch.reset();
        order_manager.reset();
        parser_fix.reset();
        login.reset();
        builder_fix.reset();
        sbt.reset();
        sess_mngr.reset();
        marketbook.reset();
        hashtables.reset();
        inPkt_pool.reset();    
    }
};

BENCHMARK_DEFINE_F(BM_Builder, MixedTraffic)(benchmark::State& state)
{
    pin_to_cpu(2);
    
    auto& seq_fix = sess_mngr->getSessionState(sess_index)->fix;
    
    std::vector<uint64_t> latencies;
    latencies.reserve(100000);

    for(auto _ : state)
    {
        seq_fix.set_next_seq(1);
        
        for (auto ord : orders) 
            risk_to_builder.push(ord); 
        
        asm volatile("" ::: "memory");
        auto wall_start = std::chrono::high_resolution_clock::now();
        uint64_t start = rdtsc_start();

        builder_dispatch->dispatch();

        uint64_t end = rdtsc_end();
        auto wall_end = std::chrono::high_resolution_clock::now();
        
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(wall_end - wall_start).count();
        state.SetIterationTime(elapsed * 1e-9);
        
        benchmark::ClobberMemory();
        latencies.push_back(end - start);
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

BENCHMARK_REGISTER_F(BM_Builder, MixedTraffic)->UseManualTime();

   
  
    


   


