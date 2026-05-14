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
    std::unique_ptr<InPacketPoolManager> inPkt_pool;
    std::unique_ptr<SessionManager> sess_mngr;
    std::unique_ptr<SoupBinTcp> sbt;
    std::unique_ptr<Builder_FIX> builder_fix;
    std::unique_ptr<LoginController> login;
    std::unique_ptr<NetworkIO> network_io;
    std::unique_ptr<Parser_FIX> parser_fix;

    spscFIXInSessionQueue_t parser_to_fixbuilder_in;
    spscOutPacketQueue_t receiver_to_parser;
    spscInPacketQueue_t builder_to_sender;
    spscMessageQueue_t parser_to_store; 
    spscFIXOutSessionQueue_t parser_to_fixbuilder_out;
    spscDbQueue_t db_to_parser; 

    std::unique_ptr<Parser_Dispatch> parser_dispatch;

    SessionState* sess_state;
    OutPacket* pkt;
    std::vector<OutPacket*> pkts;
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
        
        inPkt_pool = std::make_unique<InPacketPoolManager>();

        sess_mngr   = std::make_unique<SessionManager>();
        sbt         = std::make_unique<SoupBinTcp>(*sess_mngr);
        builder_fix = std::make_unique<Builder_FIX>(*sess_mngr);
        login       = std::make_unique<LoginController>(*sbt, *builder_fix, *sess_mngr);
        parser_fix  = std::make_unique<Parser_FIX>(parser_to_fixbuilder_in);

        network_io  = std::make_unique<NetworkIO>(
                                        receiver_to_parser,
                                        builder_to_sender,
                                        *sess_mngr,
                                        *sbt,
                                        *login,
                                        *inPkt_pool,
                                        running
            );

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

        uint8_t sess_index = sess_mngr->getSessionIndex(Venue::BIST, Protocol::FIX);
        sess_state = sess_mngr->getSessionState(sess_index);
        
        pkt_case = st.range(0);
        switch(pkt_case)
        {
            case 1: 
                pkts.push_back(&test_data_parser::fix_outpacket_single_1); // 1msg 1pkt
                break;
            case 2: 
                pkts.push_back(&test_data_parser::fix_outpacket_full_1); // 3msgs 1pkt
                break;
            case 3: 
                pkts.push_back(&test_data_parser::fix_outpacket_partial_1); // 3msgs scattered 3pkts, in order
                pkts.push_back(&test_data_parser::fix_outpacket_partial_2);
                pkts.push_back(&test_data_parser::fix_outpacket_partial_3); 
                break;
            case 4: 
                pkts.push_back(&test_data_parser::fix_outpacket_partial_4); // 3msgs scattered 3pkts, out of order
                pkts.push_back(&test_data_parser::fix_outpacket_partial_5);
                pkts.push_back(&test_data_parser::fix_outpacket_partial_6); 
                break;
            default: 
                __builtin_unreachable(); 
        }  

        consumer = std::thread([&]
        {
            pin_to_cpu(0);        

            MessageWithVenue<MessageTypes_t> local_msg;
            FIXSessionMessage* sesMsg;  
            
            while(!stop.load(std::memory_order_acquire))
            {
                if(parser_to_store.pop(local_msg)) 
                {
                    parser_dispatch->fixparser_.releaseFIX(std::get<FIXMessage*>(local_msg.msg));
                    
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
        consumer.join();

        parser_dispatch.reset();
        network_io.reset();
        login.reset();
        builder_fix.reset();
        sbt.reset();
        sess_mngr.reset();
        inPkt_pool.reset();

    }
};

BENCHMARK_DEFINE_F(BM_Parser, ParseFIX)(benchmark::State& state)
{
    pin_to_cpu(2);

    std::vector<uint64_t> latencies;
    latencies.reserve(100000);
    
    for(auto _ : state)
    {
        for(auto pkt : pkts) 
            pkt->consumed = false;
        
        sess_state->fix.set_expected_seq(pkt_case == 4 ? 4 : 1);
        
        asm volatile("" ::: "memory");
        auto wall_start = std::chrono::high_resolution_clock::now();
        uint64_t start = rdtsc_start();

        for (auto* pkt : pkts)
            parser_dispatch->parseFIX(pkt);

        uint64_t end = rdtsc_end();
        auto wall_end = std::chrono::high_resolution_clock::now();
        
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(wall_end - wall_start).count();
        state.SetIterationTime(elapsed * 1e-9);
        benchmark::ClobberMemory();
        latencies.push_back(end - start);

        while (!parser_to_store.empty() || !parser_to_fixbuilder_in.empty())
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

BENCHMARK_REGISTER_F(BM_Parser, ParseFIX)->Arg(0)->UseManualTime()->Name("BM_ParserFIX_best");
BENCHMARK_REGISTER_F(BM_Parser, ParseFIX)->Arg(1)->UseManualTime()->Name("BM_ParserFIX_normal");
BENCHMARK_REGISTER_F(BM_Parser, ParseFIX)->Arg(2)->UseManualTime()->Name("BM_ParserFIX_worst(seq-in-order)");
BENCHMARK_REGISTER_F(BM_Parser, ParseFIX)->Arg(3)->UseManualTime()->Name("BM_ParserFIX_worst(seq-out-of-order)");