
#include <benchmark/benchmark.h>

#include "OrderManager.h"
#include "MarketBook.h"
#include "HashTables.h"
#include "Order.h"
#include "dataset_risk.h"
#include "dataset_order_manager.h"
#include "RiskEngine.h"
#include "Limits.h"
#include "Logger.h"
#include "bench_utils.h"

#include <thread>
#include <atomic>
#include <vector>
#include <array>
#include <memory>
#include <chrono>

inline void reset_accountrisk(AccountRisk& ar) noexcept;
inline void reset_symbolrisk(SymbolRisk& sr) noexcept;

class BM_Risk : public benchmark::Fixture
{
public:
    std::unique_ptr<HashTables> hashtables;
    std::unique_ptr<Limits> limits;
    std::unique_ptr<MarketBook> marketbook;
    std::unique_ptr<OrderManager> order_manager;
    std::unique_ptr<RiskEngine> risk;

    spscMessageQueue_t parser_to_store;
    spscOrderQueue_t strategy_to_risk;
    spscRejectOrderQueue_t risk_to_strategy;
    spscOrderQueue_t risk_to_builder;
    spscOrderQueue_t store_to_strategy_free_slot;
    spscOrderQueue_t store_to_risk;
    spscOrderQueue_t store_to_strategy;
    spscDbQueue_t store_to_db;

    std::atomic<bool> stop{false};
    std::thread consumer;

    std::atomic<bool> stop2{false};
    std::thread consumer2;

public:
    void SetUp(const ::benchmark::State& st) 
    {
        stop.store(false, std::memory_order_release);
        stop2.store(false, std::memory_order_release);
        
        hashtables    = std::make_unique<HashTables>();
        limits        = std::make_unique<Limits>(*hashtables);
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

        risk          = std::make_unique<RiskEngine>(
                                        store_to_risk,
                                        strategy_to_risk,
                                        risk_to_strategy,
                                        risk_to_builder,
                                        *hashtables,
                                        *marketbook,
                                        *limits,
                                        *order_manager   

        );

        //SYMBOLMETA INITIALIZING  (BIST GARAN(order_book_id = 3) and NASDAQ AAPL(stock_locate = 1))
        parser_to_store.push(MessageWithVenue<MessageTypes_t>{NASDAQ::ITCHMessage{&test_data_ordMngr::nasdaq_dir}, Venue::NASDAQ});
        order_manager->store();
        
        parser_to_store.push(MessageWithVenue<MessageTypes_t>{BIST::ITCHMessage{&test_data_ordMngr::bist_dir}, Venue::BIST});
        order_manager->store();

        //TICK SIZE INITIALIZATION VENUE::BIST--0 SYMBOL::GARAN--0 TICKSIZE--10
        auto* symmeta = order_manager->get_symbolmeta(Venue::BIST, 3);
        auto* ticksize_entry = new TickSizeEntry(0, 100'000'000, 10);
        symmeta->tick_size_table[0].store(ticksize_entry, std::memory_order_relaxed);

        // BEST BID/ASK INITIALIZATION VENUE::BIST--0 SYMBOL::GARAN--0
        auto& symRisk = risk->symbolrisks_[0][0];
        symRisk.best_bid.store(10000, std::memory_order_relaxed);
        symRisk.best_ask.store(10000, std::memory_order_relaxed);

        consumer = std::thread([&]
        {
            pin_to_cpu(0);        

            Order* order; 
            
            while(!stop.load(std::memory_order_acquire))
            {
                if(!risk_to_builder.pop(order)) 
                {
                    _mm_pause();  
                }
            }
        });

        consumer2 = std::thread([&]
        {
            pin_to_cpu(4);        

            OrderWithRejectReason orderWRR; 
            
            while(!stop2.load(std::memory_order_acquire))
            {
                if(!risk_to_strategy.pop(orderWRR)) 
                    _mm_pause();  
            }
        });
    }

    void TearDown(const ::benchmark::State& st) 
    {
        stop.store(true, std::memory_order_release);
        stop2.store(true, std::memory_order_release);
        consumer.join();
        consumer2.join();

        order_manager.reset();
        marketbook.reset();
        limits.reset();
        hashtables.reset();
    }
};

//========================================================
//                  RISK UPDATE BENCHMARK
//========================================================
    BENCHMARK_DEFINE_F(BM_Risk, Update)(benchmark::State& state)
    {
        pin_to_cpu(2);
        
        int order_case = state.range(0);

        std::vector<uint64_t> latencies;
        latencies.reserve(100000);

        for(auto _ : state)
        {
            if(order_case == 0) 
            { 
                store_to_risk.push(test_data_risk::fixNew);
            }
            else
            {
                store_to_risk.push(test_data_risk::fixNew);
                store_to_risk.push(test_data_risk::fixPartial);
                store_to_risk.push(test_data_risk::fixCancel);
            }
        

            bool processing = true;

            asm volatile("" ::: "memory");
            auto wall_start = std::chrono::high_resolution_clock::now();
            uint64_t start = rdtsc_start();

            while (processing)
                processing = risk->update_risk();

            uint64_t end = rdtsc_end();
            auto wall_end = std::chrono::high_resolution_clock::now();
            
            auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(wall_end - wall_start).count();
            state.SetIterationTime(elapsed * 1e-9);
            
            benchmark::ClobberMemory();
            latencies.push_back(end - start);

            reset_accountrisk(risk->accountrisks_[0]);
            reset_symbolrisk(risk->symbolrisks_[0][0]);
            risk->orderrisks_[0].clear();
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

//========================================================
//                  RISK CHECK BENCHMARK
//========================================================
    BENCHMARK_DEFINE_F(BM_Risk, Check)(benchmark::State& state)
    {
        pin_to_cpu(2);

        std::vector<uint64_t> latencies;
        latencies.reserve(100000);

        for(auto _ : state)
        {
            strategy_to_risk.push(test_data_risk::o1);
            bool processing = true;
            
            asm volatile("" ::: "memory");
            auto wall_start = std::chrono::high_resolution_clock::now();
            uint64_t start = rdtsc_start();

            while (processing)
                processing = risk->check_risk();

            uint64_t end = rdtsc_end();
            auto wall_end = std::chrono::high_resolution_clock::now();
            
            auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(wall_end - wall_start).count();
            state.SetIterationTime(elapsed * 1e-9);
            
            benchmark::ClobberMemory();
            latencies.push_back(end - start);
            
            reset_accountrisk(risk->accountrisks_[0]);
            reset_symbolrisk(risk->symbolrisks_[0][0]);
            risk->orderrisks_[0].clear();
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


BENCHMARK_REGISTER_F(BM_Risk, Update)->Arg(0)->UseManualTime()->Name("BM_Risk_Update_single");
BENCHMARK_REGISTER_F(BM_Risk, Update)->Arg(1)->UseManualTime()->Name("BM_Risk_Update_flow");
BENCHMARK_REGISTER_F(BM_Risk, Check)->UseManualTime()->Name("BM_Risk_Check_single");


inline void reset_accountrisk(AccountRisk& ar) noexcept
{
    ar.current_exposure.store(0, std::memory_order_relaxed);
    ar.balance.store(10'000'000, std::memory_order_relaxed);
    ar.positional_exposure.store(0, std::memory_order_relaxed);
    ar.used_margin.store(0, std::memory_order_relaxed);
    ar.current_leverage.store(1, std::memory_order_relaxed);
    ar.daily_realized_pnl.store(0, std::memory_order_relaxed);
    ar.total_unrealized_pnl.store(0, std::memory_order_relaxed);
    ar.open_orders_count.store(0, std::memory_order_relaxed);
}

inline void reset_symbolrisk(SymbolRisk& sr) noexcept
{
    sr.net_position.store(0, std::memory_order_relaxed);
    sr.cost_basis_scaled.store(0, std::memory_order_relaxed);
    sr.unrealized_pnl.store(0, std::memory_order_relaxed);
    sr.realized_pnl.store(0, std::memory_order_relaxed);
    sr.pending_notional_scaled.store(0, std::memory_order_relaxed);
    sr.best_bid.store(10000, std::memory_order_relaxed);
    sr.best_ask.store(10000, std::memory_order_relaxed);
    sr.avg_entry_price.store(0, std::memory_order_relaxed);
    sr.open_orders_count.store(0, std::memory_order_relaxed);
}