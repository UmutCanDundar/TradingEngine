#include <benchmark/benchmark.h>

#include "SessionManager.h"
#include "SoupBinTcp.h"
#include "Builder_OUCH_NASDAQ.h"
#include "dataset_builder.h"
#include "bench_utils.h"

static void BM_Builder_OUCH_NASDAQ(benchmark::State& state)
{
    pin_to_cpu(6);

    auto ord_case = state.range(0);
    
    std::vector<Order*> orders; 

    auto sess_mngr   = std::make_unique<SessionManager>();
    auto sbt         = std::make_unique<SoupBinTcp>(*sess_mngr);
    auto builder_ouch_nasdaq = std::make_unique<Builder_OUCH_NASDAQ>(*sbt);

    std::vector<uint64_t> latencies;
    latencies.reserve(1000000);

    for (auto _ : state)
    {
        orders.clear();
        if(!ord_case)
        {
            orders.push_back(test_data_builder::NQouch_new_order);
        }
        else
        {
            orders.push_back(test_data_builder::NQouch_new_order);
            orders.push_back(test_data_builder::NQouch_replace_order);
            orders.push_back(test_data_builder::NQouch_modify_order);
            orders.push_back(test_data_builder::NQouch_new_order);
            orders.push_back(test_data_builder::NQouch_new_order);
            orders.push_back(test_data_builder::NQouch_cancel_order);
            orders.push_back(test_data_builder::NQouch_new_order);
            orders.push_back(test_data_builder::NQouch_new_order);
        }

        asm volatile("" ::: "memory");
        uint64_t start = rdtsc_start();

        for(auto* ord : orders)
            builder_ouch_nasdaq->build(*ord);

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

BENCHMARK(BM_Builder_OUCH_NASDAQ)->Arg(0)->Name("BM_Builder_OUCH_NASDAQ_newOrder");
BENCHMARK(BM_Builder_OUCH_NASDAQ)->Arg(1)->Name("BM_Builder_OUCH_NASDAQ_mixedTraffic");  



