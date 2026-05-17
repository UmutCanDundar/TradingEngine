#include <benchmark/benchmark.h>

#include "Parser_Dispatch.h"
#include "SessionManager.h"
#include "LoginController.h"
#include "SoupBinTcp.h"
#include "Builder_FIX.h"
#include "Parser_FIX.h"
#include "NetworkIO.h"
#include "dataset_parser.h"
#include "bench_utils.h"

#include <thread>
#include <atomic>
#include <vector>
#include <array>
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
    std::unique_ptr<Parser_Dispatch> parser_dispatch;

    spscFIXInSessionQueue_t parser_to_fixbuilder_in;
    spscRxPacketQueue_t receiver_to_parser;
    spscTxPacketQueue_t builder_to_sender;
    spscMessageQueue_t parser_to_store; 
    spscFIXOutSessionQueue_t parser_to_fixbuilder_out;
    spscDbQueue_t db_to_parser; 

    std::atomic<bool> stop{false};
    std::thread consumer;

    std::atomic<bool> stop2{false};
    std::thread network_thread;
    std::atomic<bool> trigger_network{false};
    std::atomic<bool> running{true};
    
    std::vector<RxPacket*> pkts;
    size_t sess_index_fix;
    size_t sess_index_ouch;
    PendingQueue<RxPacket *, 256> pend_read;
    
public:
    
    void SetUp(const ::benchmark::State& st) 
    {
        stop.store(false, std::memory_order_release);
        stop2.store(false, std::memory_order_relaxed);
        
        txPkt_pool = std::make_unique<TxPacketPoolManager>();
        sess_mngr     = std::make_unique<SessionManager>();
        sbt           = std::make_unique<SoupBinTcp>(*sess_mngr);
        builder_fix   = std::make_unique<Builder_FIX>(*sess_mngr);
        login         = std::make_unique<LoginController>(*sbt, *builder_fix, *sess_mngr);
        network_io    = std::make_unique<NetworkIO>(receiver_to_parser, builder_to_sender, *sess_mngr, *sbt, *login, *txPkt_pool, running);
        parser_fix    = std::make_unique<Parser_FIX>(parser_to_fixbuilder_in);

        parser_dispatch = std::make_unique<Parser_Dispatch>(
                                            receiver_to_parser,
                                            parser_to_store,
                                            parser_to_fixbuilder_out,
                                            parser_to_fixbuilder_in,
                                            *sess_mngr,
                                            db_to_parser,
                                            *network_io,
                                            *parser_fix
        );

        sess_index_fix = sess_mngr->getSessionIndex(Venue::BIST, Protocol::FIX);
        sess_index_ouch = sess_mngr->getSessionIndex(Venue::BIST, Protocol::OUCH);

        pkts = {
            &test_data_parser::ouch_bist_RxPacket_partial_1, 
            &test_data_parser::ouch_bist_RxPacket_partial_2,
            &test_data_parser::itch_bist_RxPacket_single_1,
            &test_data_parser::fix_RxPacket_partial_1, 
            &test_data_parser::fix_RxPacket_partial_2,
            &test_data_parser::ouch_bist_RxPacket_partial_3,
            &test_data_parser::ouch_bist_RxPacket_partial_4,  
            &test_data_parser::fix_RxPacket_partial_3,
            &test_data_parser::fix_RxPacket_partial_4,
            &test_data_parser::ouch_bist_RxPacket_partial_5,
            &test_data_parser::itch_nasdaq_RxPacket_single_1,  
            &test_data_parser::fix_RxPacket_partial_5, 
            &test_data_parser::fix_RxPacket_partial_6,   
        };

        network_thread = std::thread([&]()
        {
            pin_to_cpu(4);
            
            while(!stop2.load(std::memory_order_acquire))
            {
                if(trigger_network.load(std::memory_order_acquire))
                {
                    for (auto pkt : pkts) 
                    {
                        pkt->consumed = false;
                        pkt->msg_count = 0;
                        
                        if(pkt->protocol == Protocol::OUCH) 
                        {
                            bool ouch_rxPkt_ready = false; 
                                    
                            ouch_rxPkt_ready = network_io->SBTPacketHandler(pkt, pkt->len, pend_read, sess_index_ouch);
                            
                            if(ouch_rxPkt_ready)
                            {
                                receiver_to_parser.push(pkt);
                            }
                        }
                        else
                        {
                            receiver_to_parser.push(pkt);
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
            pin_to_cpu(0);        

            MessageWithVenue<MessageTypes_t> local_msg;
            FIXSessionMessage* sesMsg;   
            
            while(!stop.load(std::memory_order_acquire))
            {
                if(parser_to_store.pop(local_msg)) 
                {
                    std::visit([&](auto &msg)
                    {
                        using T = std::decay_t<decltype(msg)>;
                        if constexpr (std::is_same_v<T, FIXMessage*>)
                        {
                            parser_dispatch->fixparser_.releaseFIX(msg);
                        }
                        else if constexpr (std::is_same_v<T, BIST::ITCHMessage>)
                        {
                            parser_dispatch->itchparser_bist_.releaseITCH(msg);
                        }
                        else if constexpr (std::is_same_v<T, BIST::OUCHMessage>)
                        {
                            parser_dispatch->ouchparser_bist_.releaseOUCH(msg);
                        }
                        else if constexpr (std::is_same_v<T, NASDAQ::ITCHMessage>)
                        {
                            parser_dispatch->itchparser_nasdaq_.releaseITCH(msg);
                        }
                    },
                    local_msg.msg);

                }
                else if(parser_to_fixbuilder_in.pop(sesMsg))
                {
                    parser_dispatch->fixparser_.releaseFIX(sesMsg);
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
        parser_fix.reset();
        network_io.reset();
        login.reset();
        builder_fix.reset();
        sbt.reset();
        sess_mngr.reset();
        txPkt_pool.reset();    
    }
};

BENCHMARK_DEFINE_F(BM_Parser, MixedTraffic)(benchmark::State& state)
{
    pin_to_cpu(2);
    
    auto& seq_fix = sess_mngr->getSessionState(sess_index_fix)->fix;
    
    std::vector<uint64_t> latencies;
    latencies.reserve(100000);

    for(auto _ : state)
    {          
        trigger_network.store(true, std::memory_order_release);

        while (trigger_network.load(std::memory_order_acquire)) _mm_pause();
        std::atomic_thread_fence(std::memory_order_seq_cst);

        seq_fix.set_expected_seq(1);

        bool processing = true;

        asm volatile("" ::: "memory");
        auto wall_start = std::chrono::high_resolution_clock::now();
        uint64_t start = rdtsc_start();

        while(processing)
            processing = parser_dispatch->dispatch();

        uint64_t end = rdtsc_end();
        auto wall_end = std::chrono::high_resolution_clock::now();
        
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(wall_end - wall_start).count();
        state.SetIterationTime(elapsed * 1e-9);
        
        benchmark::ClobberMemory();
        latencies.push_back(end - start);

        while(!parser_to_store.empty() || !receiver_to_parser.empty()) _mm_pause();
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

BENCHMARK_REGISTER_F(BM_Parser, MixedTraffic)->UseManualTime();

   
  
   







