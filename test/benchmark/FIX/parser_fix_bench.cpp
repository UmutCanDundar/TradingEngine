#include <benchmark/benchmark.h>

#include "Parser_FIX.h"
#include "dataset.h"
#include "bench_utils.h"

static void BM_Parse_Pkt1(benchmark::State& state)
{
    pin_to_cpu(0);

    bool cold = state.range(0);

    spscFIXInSessionQueue_t queue;
    Parser_FIX parser{queue};

    auto data = test_data::single_fix_pkt1.data();
    auto size = test_data::single_fix_pkt1.size();

    std::vector<uint64_t> latencies;
    latencies.reserve(100000);

    for (auto _ : state)
    {
        if (cold) flush_cache();
        asm volatile("" ::: "memory");
        uint64_t start = rdtsc_start();

        auto* msg = parser.parse<FIXMessage>(
            reinterpret_cast<const char*>(data),
            size
        );

        benchmark::DoNotOptimize(msg);

        uint64_t end = rdtsc_end();

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

    state.counters["p50_cycles"] = percentile(0.50);
    state.counters["p95_cycles"] = percentile(0.95);
    state.counters["p99_cycles"] = percentile(0.99);
}

static void BM_Parse_Pkt2(benchmark::State& state)
{
    pin_to_cpu(0);

    bool cold = state.range(0);

    spscFIXInSessionQueue_t queue;
    Parser_FIX parser{queue};

    auto data = test_data::single_fix_pkt2.data();
    auto size = test_data::single_fix_pkt2.size();

    std::vector<uint64_t> latencies;
    latencies.reserve(100000);

    for (auto _ : state)
    {
        if (cold) flush_cache();
        asm volatile("" ::: "memory");  
        uint64_t start = rdtsc_start();

        auto* msg = parser.parse<FIXMessage>(
            reinterpret_cast<const char*>(data),
            size
        );

        benchmark::DoNotOptimize(msg);

        uint64_t end = rdtsc_end();

        latencies.push_back(end - start);

        parser.releaseFIX(msg);
        
        benchmark::ClobberMemory();
    }

    std::sort(latencies.begin(), latencies.end());

    auto percentile = [&](double p)
    {
        size_t idx = std::min(
            static_cast<size_t>(p * latencies.size()),
            latencies.size() - 1
        );
        return latencies[idx];
    };

    state.counters["p50_cycles"] = percentile(0.50);
    state.counters["p95_cycles"] = percentile(0.95);
    state.counters["p99_cycles"] = percentile(0.99);
}


BENCHMARK(BM_Parse_Pkt1)->Arg(0); // Warm
BENCHMARK(BM_Parse_Pkt1)->Arg(1); // Cold

BENCHMARK(BM_Parse_Pkt2)->Arg(0); // Warm
BENCHMARK(BM_Parse_Pkt2)->Arg(1); // Cold