#include <benchmark/benchmark.h>

#include "Parser_Dispatch.h"
#include "SessionManager.h"
#include "LoginController.h"
#include "SoupBinTcp.h"
#include "Builder_FIX.h"
#include "NetworkIO.h"
#include "Parser_FIX.h"
#include "Logger.h"
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

    
    std::vector<RxPacket*> pkts;
    int pkt_case;
    ssize_t rest_ouch_msg;
    size_t ouch_msg_count;
    bool sbt_ret = true;

    std::atomic<bool> stop{false};
    std::thread consumer;
    
    std::atomic<bool> stop2{false};
    std::thread network_thread;
    std::atomic<bool> trigger_network{false};
    std::atomic<bool> running{true};
    
    PendingQueue<RxPacket *, 256> pend_read;
    uint8_t sess_index;

public:
    void SetUp(const ::benchmark::State& st) 
    {
        stop.store(false, std::memory_order_relaxed);
        stop2.store(false, std::memory_order_relaxed);
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

        sess_index = sess_mngr->getSessionIndex(Venue::BIST, Protocol::OUCH);
        pkt_case = st.range(0);
        switch(pkt_case)
        {
            case 1: 
                pkts.push_back(&test_data_parser::ouch_bist_RxPacket_single_1); // 1msg 1pkt
                rest_ouch_msg = 1;
                ouch_msg_count = 1; 
                break;
            case 2: 
                pkts.push_back(&test_data_parser::ouch_bist_RxPacket_full_1); // 3msgs 1pkt
                rest_ouch_msg = 3;
                ouch_msg_count = 3; 
                break;
            case 3: 
                pkts.push_back(&test_data_parser::ouch_bist_RxPacket_partial_1); // 3msgs scattered 3pkts
                pkts.push_back(&test_data_parser::ouch_bist_RxPacket_partial_2);
                pkts.push_back(&test_data_parser::ouch_bist_RxPacket_partial_3); 
                rest_ouch_msg = 3;
                ouch_msg_count = 3; 
                break;
            default: 
            __builtin_unreachable();
        }

        network_thread = std::thread([&]()
        {
            pin_to_cpu(14);
            
            while(!stop2.load(std::memory_order_acquire))
            {
                if(trigger_network.load(std::memory_order_acquire))
                {
                    for (auto* pkt : pkts)
                    {
                        sbt_ret = true;

                        for(size_t i = 0; i < ouch_msg_count; i++)
                        {
                            if(!sbt_ret)
                                break;

                            sbt_ret = network_io->SBTPacketHandler(pkt, pkt->len, pend_read, sess_index);
                            if(sbt_ret) 
                            {
                                receiver_to_parser.push(pkt);
                            }
                        }
                    }

                    trigger_network.store(false, std::memory_order_release);
                }
                else
                {
                    _mm_pause();
                }           
            }
        });

        consumer = std::thread([&]
        {
            pin_to_cpu(15);        

            MessageWithVenue<MessageTypes_t> local_msg;
            
            while(!stop.load(std::memory_order_acquire))
            {
                if (!parser_to_store.empty())
                {
                    parser_to_store.pop(local_msg);
                    parser_dispatch->ouchparser_bist_.releaseOUCH(std::get<BIST::OUCHMessage>(local_msg.msg));

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
        stop2.store(true, std::memory_order_release);
        
        consumer.join();
        network_thread.join();

        parser_dispatch.reset();
        network_io.reset();
        login.reset();
        builder_fix.reset();
        sbt.reset();
        sess_mngr.reset();
        txPkt_pool.reset();
    }
};

BENCHMARK_DEFINE_F(BM_Parser, ParseOUCH_BIST)(benchmark::State& state)
{
    pin_to_cpu(6);

    std::vector<uint64_t> latencies;
    latencies.reserve(100000);
    
    for(auto _ : state)
    {
        while (trigger_network.load(std::memory_order_acquire)) _mm_pause();
        std::atomic_thread_fence(std::memory_order_seq_cst);
        
        for(auto* pkt : pkts)
        {
            pkt->msg_count = 0;
            pkt->consumed = false;
        }
        
        rest_ouch_msg = (pkt_case == 1) ? 1 : 3;
        
        asm volatile("" ::: "memory");
        auto wall_start = std::chrono::high_resolution_clock::now();
        uint64_t start = rdtsc_start();

        trigger_network.store(true, std::memory_order_release);
       
        while(rest_ouch_msg > 0)
        {
            if(parser_dispatch->dispatch())
            {                
                rest_ouch_msg--;
            } 
            else 
            {
                _mm_pause(); 
            }
        }
        
        uint64_t end = rdtsc_end();
        auto wall_end = std::chrono::high_resolution_clock::now();
        
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(wall_end - wall_start).count();
        state.SetIterationTime(elapsed * 1e-9);
        benchmark::ClobberMemory();
        latencies.push_back(end - start);
        
        while (!parser_to_store.empty()) _mm_pause();
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

BENCHMARK_REGISTER_F(BM_Parser, ParseOUCH_BIST)->Arg(1)->UseManualTime()->Name("BM_ParserOUCH_BIST_best");
BENCHMARK_REGISTER_F(BM_Parser, ParseOUCH_BIST)->Arg(2)->UseManualTime()->Name("BM_ParserOUCH_BIST_normal");
BENCHMARK_REGISTER_F(BM_Parser, ParseOUCH_BIST)->Arg(3)->UseManualTime()->Name("BM_ParserOUCH_BIST_worst");
