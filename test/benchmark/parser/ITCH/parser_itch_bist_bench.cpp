#include <benchmark/benchmark.h>

#include "Parser_Dispatch.h"
#include "SessionManager.h"
#include "LoginController.h"
#include "SoupBinTcp.h"
#include "Builder_FIX.h"
#include "NetworkIO.h"
#include "Parser_FIX.h"
#include "dataset_parser.h"
#include "bench_utils.h"

#include <thread>
#include <atomic>
#include <vector>
#include <memory>
#include <chrono>

class BM_Parser : public benchmark::Fixture
{
public:
    std::unique_ptr<TxPacketPoolManager> txPkt_pool;
    std::unique_ptr<SessionManager> sess_mngr;
    std::unique_ptr<SoupBinTcp> sbt;
    std::unique_ptr<Builder_FIX> builder_fix;
    std::unique_ptr<LoginController> login;
    std::unique_ptr<NetworkIO> network_io;
    std::unique_ptr<Parser_FIX> parser_fix;

    spscFIXTxSessionQueue_t parser_to_fixbuilder_tx;
    spscRxPacketQueue_t receiver_to_parser;
    spscTxPacketQueue_t builder_to_sender;
    spscMessageQueue_t parser_to_store; 
    spscFIXRxSessionQueue_t parser_to_fixbuilder_rx;
    spscDbQueue_t db_to_parser; 

    std::unique_ptr<Parser_Dispatch> parser_dispatch;

    RxPacket* pkt;
    std::vector<RxPacket*> pkts;
    int pkt_case;
    MessageWithVenue<MessageTypes_t> msg;
    std::atomic<bool> running{true}; 

    std::atomic<bool> stop{false};
    std::thread consumer;

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
        parser_fix  = std::make_unique<Parser_FIX>(parser_to_fixbuilder_tx);

        network_io  = std::make_unique<NetworkIO>(
                                        receiver_to_parser,
                                        builder_to_sender,
                                        *sess_mngr,
                                        *sbt,
                                        *login,
                                        *txPkt_pool,
                                        running
            );

        parser_dispatch = std::make_unique<Parser_Dispatch>(
                                            receiver_to_parser,
                                            parser_to_store,
                                            parser_to_fixbuilder_rx,
                                            parser_to_fixbuilder_tx,
                                            *sess_mngr,
                                            db_to_parser,
                                            *network_io,
                                            *parser_fix
        );
        
        pkt_case = st.range(0);
        switch(pkt_case)
        {
            case 1: 
                pkts.push_back(&test_data_parser::itch_bist_RxPacket_single_1); // 1msg 1pkt
                break;
            case 2: 
                pkts.push_back(&test_data_parser::itch_bist_RxPacket_full_1); // 3msg 1pkt
                break;
            default: 
            __builtin_unreachable();
        }

        consumer = std::thread([&]
        {
            pin_to_cpu(15);        

            MessageWithVenue<MessageTypes_t> local_msg;
            
            while(!stop.load(std::memory_order_acquire))
            {
                if (!parser_to_store.empty())
                {
                    parser_to_store.pop(local_msg);
                    parser_dispatch->itchparser_bist_.releaseITCH(std::get<BIST::ITCHMessage>(local_msg.msg));
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

        parser_dispatch.reset();
        network_io.reset();
        login.reset();
        builder_fix.reset();
        sbt.reset();
        sess_mngr.reset();
        txPkt_pool.reset();
    }
};

BENCHMARK_DEFINE_F(BM_Parser, ParseITCH_BIST)(benchmark::State& state)
{
    pin_to_cpu(6);

    std::vector<uint64_t> latencies;
    latencies.reserve(100000);
    
    for(auto _ : state)
    {
        for(auto* pkt : pkts)
            pkt->msg_count = 0;
        
        asm volatile("" ::: "memory");
        auto wall_start = std::chrono::high_resolution_clock::now();
        uint64_t start = rdtsc_start();

        for (auto* pkt : pkts)
            parser_dispatch->parseITCH_BIST(pkt);

        uint64_t end = rdtsc_end();
        auto wall_end = std::chrono::high_resolution_clock::now();
        
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(wall_end - wall_start).count();
        state.SetIterationTime(elapsed * 1e-9);
        benchmark::ClobberMemory();
        latencies.push_back(end - start);

        while (!parser_to_store.empty())
            _mm_pause();
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

BENCHMARK_REGISTER_F(BM_Parser, ParseITCH_BIST)->Arg(1)->UseManualTime()->Name("BM_ParserITCH_BIST_best");
BENCHMARK_REGISTER_F(BM_Parser, ParseITCH_BIST)->Arg(2)->UseManualTime()->Name("BM_ParserITCH_BIST_worst");
