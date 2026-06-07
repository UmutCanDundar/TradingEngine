#include <benchmark/benchmark.h>

#include "Parser_FIX.h"
#include "dataset_parser.h"
#include "bench_utils.h"

static void BM_Parser_FIX_parseFunc(benchmark::State& state)
{
    pin_to_cpu(0);

    const uint64_t pkt_idx = state.range(0); 

    const uint8_t* data = (pkt_idx == 1)
        ? test_data_parser::single_fix_pkt1.data()
        : test_data_parser::single_fix_pkt2.data();

    const size_t size = (pkt_idx == 1)
        ? test_data_parser::single_fix_pkt1.size()
        : test_data_parser::single_fix_pkt2.size();

    spscFIXTxSessionQueue_t queue;
    Parser_FIX parser{queue};

    std::vector<uint64_t> latencies;
    latencies.reserve(1000000);

    for (auto _ : state)
    {
        asm volatile("" ::: "memory");
        uint64_t start = rdtsc_start();

        auto* msg = parser.parse<FIXMessage>(
            reinterpret_cast<const char*>(data),
            size
        );

        uint64_t end = rdtsc_end();
        benchmark::DoNotOptimize(msg);

        latencies.push_back(end - start);

        parser.releaseFIX(msg);
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

BENCHMARK(BM_Parser_FIX_parseFunc)->Arg(1)->ArgName("NewOrder");  
BENCHMARK(BM_Parser_FIX_parseFunc)->Arg(2)->ArgName("ExecReport");  


