#include <benchmark/benchmark.h>

#include "SessionManager.h"
#include "SoupBinTcp.h"
#include "Builder_OUCH_BIST.h"
#include "dataset_builder.h"
#include "bench_utils.h"

static void BM_Builder_OUCH_BIST(benchmark::State& state)
{
    pin_to_cpu(0);

    auto ord_case = state.range(0);
    
    std::vector<Order*> orders;

    auto sess_mngr         = std::make_unique<SessionManager>();
    auto sbt               = std::make_unique<SoupBinTcp>(*sess_mngr);
    auto builder_ouch_bist = std::make_unique<Builder_OUCH_BIST>(*sbt, *sess_mngr);

    std::vector<uint64_t> latencies;
    latencies.reserve(100000);

    for (auto _ : state)
    {

        orders.clear();
        if(!ord_case)
        {
            orders.push_back(test_data_builder::ouch_new_order);
        }
        else
        {
            orders.push_back(test_data_builder::ouch_new_order);
            orders.push_back(test_data_builder::ouch_replace_order);
            orders.push_back(test_data_builder::ouch_cancelid_order);
            orders.push_back(test_data_builder::ouch_new_order);
            orders.push_back(test_data_builder::ouch_new_order);
            orders.push_back(test_data_builder::ouch_cancel_order);
            orders.push_back(test_data_builder::ouch_new_order);
            orders.push_back(test_data_builder::ouch_new_order);
        }

        asm volatile("" ::: "memory");
        uint64_t start = rdtsc_start();
        
         for(auto* ord : orders)
            builder_ouch_bist->build(*ord);

        // for(auto* ord : orders)
        // {
        //    switch(ord->message_type)
        //     {
        //         case 0:
        //             builder_ouch_bist->build_enter(*ord);
        //             break;
        //         case 1:
        //             builder_ouch_bist->build_replace(*ord);
        //             break;
        //         case 2:
        //             builder_ouch_bist->build_cancel(*ord);
        //             break;
        //         case 3:
        //             builder_ouch_bist->build_cancelid(*ord);
        //             break;
        //     }
        // }

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


BENCHMARK(BM_Builder_OUCH_BIST)->Arg(0)->Name("BM_Builder_OUCH_BIST_newOrder");
BENCHMARK(BM_Builder_OUCH_BIST)->Arg(1)->Name("BM_Builder_OUCH_BIST_mixedTraffic");  

