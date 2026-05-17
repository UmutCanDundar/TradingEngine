#include <benchmark/benchmark.h>

#include "SessionManager.h"
#include "LoginController.h"
#include "SoupBinTcp.h"
#include "NetworkIO.h"
#include "Builder_FIX.h"
#include "dataset_parser.h"
#include "bench_utils.h"

#include <thread>
#include <atomic>
#include <vector>
#include <memory>
#include <chrono>

class BM_SBTPktHandler : public benchmark::Fixture
{
public:
    std::unique_ptr<TxPacketPoolManager> txPkt_pool;
    std::unique_ptr<SessionManager> sess_mngr;
    std::unique_ptr<SoupBinTcp> sbt;
    std::unique_ptr<Builder_FIX> builder_fix;
    std::unique_ptr<LoginController> login;
    std::unique_ptr<NetworkIO> network_io;

    spscRxPacketQueue_t receiver_to_parser;
    spscTxPacketQueue_t builder_to_sender;

    std::vector<RxPacket*> pkts;
    int pkt_case;
    bool sbt_ret = true;
    std::atomic<bool> running{true};

    std::atomic<bool> stop{false};
    std::thread consumer;
    
    PendingQueue<RxPacket *, 256> pend_read;
    uint8_t sess_index;

public:
    void SetUp(const ::benchmark::State& st) 
    {
        stop.store(false, std::memory_order_relaxed);
        pkts.clear();

        txPkt_pool = std::make_unique<TxPacketPoolManager>();
        sess_mngr   = std::make_unique<SessionManager>(); 
        sbt         = std::make_unique<SoupBinTcp>(*sess_mngr);
        builder_fix = std::make_unique<Builder_FIX>(*sess_mngr);
        login       = std::make_unique<LoginController>(*sbt, *builder_fix, *sess_mngr);

        network_io  = std::make_unique<NetworkIO>(
                                        receiver_to_parser,
                                        builder_to_sender,
                                        *sess_mngr, 
                                        *sbt,
                                        *login,
                                        *txPkt_pool,
                                        running

            );

        sess_index = sess_mngr->getSessionIndex(Venue::BIST, Protocol::OUCH);
        pkt_case = st.range(0);
        switch(pkt_case)
        {
            case 1: 
                pkts.push_back(&test_data_parser::ouch_bist_RxPacket_single_1); // 1msg 1pkt
                break;
            case 2: 
                pkts.push_back(&test_data_parser::ouch_bist_RxPacket_full_1); // 3msgs 1pkt
                break;
            case 3: 
                pkts.push_back(&test_data_parser::ouch_bist_RxPacket_partial_1); // 3msgs scattered 3pkts
                pkts.push_back(&test_data_parser::ouch_bist_RxPacket_partial_2);
                pkts.push_back(&test_data_parser::ouch_bist_RxPacket_partial_3); 
                break;
            default: 
            __builtin_unreachable();
        }

        consumer = std::thread([&]
        {
            pin_to_cpu(0);        

            RxPacket* pkt;
            RxPacket* neWpkt = new RxPacket();
            
            while(!stop.load(std::memory_order_acquire))
            {
                if (!receiver_to_parser.empty())
                {
                    receiver_to_parser.pop(pkt);
                    network_io->free_pkt_list_.push(neWpkt);
                }
                else
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

        network_io.reset();
        login.reset();
        sbt.reset();
        sess_mngr.reset();
        txPkt_pool.reset();
    }
};

BENCHMARK_DEFINE_F(BM_SBTPktHandler, DiffCase)(benchmark::State& state)
{
    pin_to_cpu(2);

    auto queue_push = state.range(1);

    std::vector<uint64_t> latencies;
    latencies.reserve(100000);
    
    for(auto _ : state)
    {
      
        for(auto* pkt : pkts)
        {
            pkt->msg_count = 0;
            pkt->consumed = false;
        }
        
        asm volatile("" ::: "memory");
        auto wall_start = std::chrono::high_resolution_clock::now();
        uint64_t start = rdtsc_start();

       
        for(auto* pkt : pkts)
        {
            sbt_ret = network_io->SBTPacketHandler(pkt, pkt->len, pend_read, sess_index);
            if(queue_push && sbt_ret) 
            { 
                receiver_to_parser.push(pkt);
            }
        }

        uint64_t end = rdtsc_end();
        auto wall_end = std::chrono::high_resolution_clock::now();
        
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(wall_end - wall_start).count();
        state.SetIterationTime(elapsed * 1e-9);
        benchmark::ClobberMemory();
        latencies.push_back(end - start);

         while(!receiver_to_parser.empty()) _mm_pause();
     
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

BENCHMARK_REGISTER_F(BM_SBTPktHandler, DiffCase)->Args({1, true})->UseManualTime()->Name("BM_SBTPktHandler_QueuePush_best");
BENCHMARK_REGISTER_F(BM_SBTPktHandler, DiffCase)->Args({1, false})->UseManualTime()->Name("BM_SBTPktHandler_noQueuePush_best");

BENCHMARK_REGISTER_F(BM_SBTPktHandler, DiffCase)->Args({2, true})->UseManualTime()->Name("BM_SBTPktHandler_QueuePush_normal");
BENCHMARK_REGISTER_F(BM_SBTPktHandler, DiffCase)->Args({2, false})->UseManualTime()->Name("BM_SBTPktHandler_noQueuePush_normal");

BENCHMARK_REGISTER_F(BM_SBTPktHandler, DiffCase)->Args({3, true})->UseManualTime()->Name("BM_SBTPktHandler_QueuePush_worst");
BENCHMARK_REGISTER_F(BM_SBTPktHandler, DiffCase)->Args({3, false})->UseManualTime()->Name("BM_SBTPktHandler_noQueuePush_worst");
