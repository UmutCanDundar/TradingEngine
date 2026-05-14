#include <benchmark/benchmark.h>

#include "SessionManager.h"
#include "Builder_FIX.h"
#include "Order.h"
#include "dataset_builder.h"
#include "bench_utils.h"

static void BM_Builder_FIX_newOrder(benchmark::State& state)
{
    pin_to_cpu(0);

    Order* order = test_data_builder::fix_new_order;
    auto sess_mngr   = std::make_unique<SessionManager>();
    auto builder_fix = std::make_unique<Builder_FIX>(*sess_mngr);
    uint8_t sess_index = sess_mngr->getSessionIndex(Venue::BIST, Protocol::FIX);
    auto& seq_fix = sess_mngr->getSessionState(sess_index)->fix;
        

    std::vector<uint64_t> latencies;
    latencies.reserve(100000);

    for (auto _ : state)
    {
        seq_fix.set_next_seq(1);

        asm volatile("" ::: "memory");
        uint64_t start = rdtsc_start();

        builder_fix->build<FIXTypes::NewOrderSingle>(sess_index, *order);

        uint64_t end = rdtsc_end();
        
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

BENCHMARK(BM_Builder_FIX_newOrder);  

