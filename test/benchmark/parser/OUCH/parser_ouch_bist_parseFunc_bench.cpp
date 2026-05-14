#include <benchmark/benchmark.h>

#include "Parser_OUCH_BIST.h"
#include "dataset_parser.h"
#include "bench_utils.h"

static void BM_Parser_OUCH_BIST_parseFunc(benchmark::State& state)
{
    pin_to_cpu(0);

    const uint64_t pkt_idx = state.range(0);  

    const uint8_t* data = [pkt_idx]()
    {
        switch(pkt_idx)
        {
            case 1: return test_data_parser::ouch_bist_pkt1.data(); 
            case 2: return test_data_parser::ouch_bist_pkt2.data(); 
            case 3: return test_data_parser::ouch_bist_pkt3.data(); 
            default: __builtin_unreachable();
        }
    }();
        
    Parser_OUCH_BIST parser{};

    std::vector<uint64_t> latencies;
    latencies.reserve(100000);

    for (auto _ : state)
    {
        asm volatile("" ::: "memory");
        uint64_t start = rdtsc_start();

        auto msg = parser.parse(reinterpret_cast<const char*>(data));

        uint64_t end = rdtsc_end();
        benchmark::DoNotOptimize(msg);

        latencies.push_back(end - start);

        parser.releaseOUCH(msg);
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

BENCHMARK(BM_Parser_OUCH_BIST_parseFunc)->Arg(1)->ArgName("Accepted");  
BENCHMARK(BM_Parser_OUCH_BIST_parseFunc)->Arg(2)->ArgName("Cancelled");
BENCHMARK(BM_Parser_OUCH_BIST_parseFunc)->Arg(3)->ArgName("Executed");    


