#include <benchmark/benchmark.h>

#include "Parser_FIX.h"
#include "dataset_parser.h"
#include "bench_utils.h"

static void BM_Parser_FIX_nextFixMsg(benchmark::State& state)
{
    pin_to_cpu(0);

    std::vector<RxPacket*> pkts;
    const auto pkt_case = state.range(0); 
    switch(pkt_case)
        {
            case 1: 
                pkts.push_back(&test_data_parser::fix_RxPacket_single_1); // 1msg 1pkt
                break;
            case 2: 
                pkts.push_back(&test_data_parser::fix_RxPacket_full_1); // 3msgs 1pkt
                break;
            case 3: 
                pkts.push_back(&test_data_parser::fix_RxPacket_partial_1); // 3msgs scattered 3pkts
                pkts.push_back(&test_data_parser::fix_RxPacket_partial_2);
                pkts.push_back(&test_data_parser::fix_RxPacket_partial_3); 
                break;          
            default: 
                __builtin_unreachable(); 
        }  

    spscFIXTxSessionQueue_t queue;
    Parser_FIX parser{queue};

    std::vector<uint64_t> latencies;
    latencies.reserve(100000);

    for (auto _ : state)
    {
        for(auto pkt : pkts) 
            pkt->consumed = false;
        
        asm volatile("" ::: "memory");
        uint64_t start = rdtsc_start();
        
        for(auto pkt : pkts)
        {
            size_t offset = 0;

            while(true)
            {   
                parser.nextFixMsg(pkt, offset);

                if(!pkt->consumed)
                    continue;

                break;
            }
        }

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

BENCHMARK(BM_Parser_FIX_nextFixMsg)->Arg(1)->Name("BM_ParserFIX_nextFixMsg_best");
BENCHMARK(BM_Parser_FIX_nextFixMsg)->Arg(2)->Name("BM_ParserFIX_nextFixMsg_normal");
BENCHMARK(BM_Parser_FIX_nextFixMsg)->Arg(3)->Name("BM_ParserFIX_nextFixMsg_worst");
