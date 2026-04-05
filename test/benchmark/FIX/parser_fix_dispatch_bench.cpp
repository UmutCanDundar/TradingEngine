#include <benchmark/benchmark.h>

#include "Parser_Dispatch.h"
#include "SessionManager.h"
#include "LoginController.h"
#include "SoupBinTcp.h"
#include "Builder_FIX.h"
#include "NetworkIO.h"
#include "dataset.h"
#include "bench_utils.h"

#include <thread>
#include <atomic>
#include <memory>

//===============================================================
//                      SINGLETHREAD BENCH
//===============================================================
class FixBench_singlethread : public benchmark::Fixture
{
public:
    // objects
    std::unique_ptr<InPacketPoolManager> inPkt_pool;
    std::unique_ptr<SessionManager> sess_mngr;
    std::unique_ptr<SoupBinTcp> sbt;
    std::unique_ptr<Builder_FIX> builder_fix;
    std::unique_ptr<LoginController> login;
    std::unique_ptr<NetworkIO> network_io;

    // queues
    spscFIXInSessionQueue_t parser_to_fixbuilder_in;
    spscOutPacketQueue_t receiver_to_parser;
    spscInPacketQueue_t builder_to_sender;
    spscMessageQueue_t parser_to_store; 
    spscFIXOutSessionQueue_t parser_to_fixbuilder_out;
    spscDbQueue_t db_to_parser; 

    // parser
    std::unique_ptr<Parser_Dispatch> parser_dispatch;

    // Helper variables
    SessionState* sess_state;
    OutPacket* pkt;
    MessageWithVenue<MessageTypes_t> msg; 

public:
    virtual void SetUp(const ::benchmark::State&)
    {
        inPkt_pool = std::make_unique<InPacketPoolManager>();

        sess_mngr   = std::make_unique<SessionManager>();  // config burada 1 kez okunur
        sbt         = std::make_unique<SoupBinTcp>(*sess_mngr);
        builder_fix = std::make_unique<Builder_FIX>(*sess_mngr);
        login       = std::make_unique<LoginController>(*sbt, *builder_fix, *sess_mngr);

        network_io  = std::make_unique<NetworkIO>(
            receiver_to_parser,
            builder_to_sender,
            *sbt,
            *sess_mngr,
            *login,
            *inPkt_pool
        );

        parser_dispatch = std::make_unique<Parser_Dispatch>(
            receiver_to_parser,
            parser_to_store,
            parser_to_fixbuilder_out,
            parser_to_fixbuilder_in,
            *sess_mngr,
            db_to_parser,
            *network_io
        );

        uint8_t sess_index = sess_mngr->getSessionIndex(Venue::BIST, Protocol::FIX);
        sess_state = sess_mngr->getSessionState(sess_index);
        pkt = &test_data::fix_outpacket;
    }

    virtual void TearDown(const ::benchmark::State&)
    {
        parser_dispatch.reset();
        network_io.reset();
        login.reset();
        builder_fix.reset();
        sbt.reset();
        sess_mngr.reset();
        inPkt_pool.reset();
    }
    
    virtual ~FixBench_singlethread() = default;
};
    
BENCHMARK_DEFINE_F(FixBench_singlethread, Parse)(benchmark::State& state)
{
    pin_to_cpu(2);
    bool cold = state.range(0);

    std::vector<uint64_t> latencies;
    latencies.reserve(100000);
    
    for(auto _ : state)
    {
        if(cold) flush_cache();

        sess_state->fix.set_expected_seq(1);
        OutPacket pkt_copy = *pkt;

        asm volatile("" ::: "memory");
        uint64_t start = rdtsc_start();

        parser_dispatch->parseFIX(&pkt_copy);

        uint64_t end = rdtsc_end();
        latencies.push_back(end - start);

        while(!parser_to_store.empty())
        {
            parser_to_store.pop(msg);
            parser_dispatch->fixparser_.releaseFIX(std::get<FIXMessage*>(msg.msg));
        } 
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

//===============================================================
//                      MULTITHREAD BENCH
//===============================================================
class FixBench_multithread : public FixBench_singlethread 
{
public:
    std::atomic<bool> stop{false};
    std::thread consumer;

public:
    void SetUp(const ::benchmark::State& st) override
    {
       stop.store(false, std::memory_order_relaxed); 
        
        FixBench_singlethread::SetUp(st);
        
        consumer = std::thread([&]
        {
            pin_to_cpu(0);        

            MessageWithVenue<MessageTypes_t> local_msg; 
            
            while(!stop.load(std::memory_order_acquire))
            {
                if(parser_to_store.pop(local_msg)) 
                {
                    parser_dispatch->fixparser_.releaseFIX(std::get<FIXMessage*>(local_msg.msg));
                } 
                else 
                {
                    _mm_pause();  
                }
            }
        });

    }

    void TearDown(const ::benchmark::State& st) override
    {
        stop.store(true, std::memory_order_release);
        consumer.join();

        FixBench_singlethread::TearDown(st);
    }
};

BENCHMARK_DEFINE_F(FixBench_multithread, Parse)(benchmark::State& state)
{
    
    pin_to_cpu(2);
    bool cold = state.range(0);

    std::vector<uint64_t> latencies;
    latencies.reserve(100000);
    
    for(auto _ : state)
    {
        if(cold) flush_cache();

        sess_state->fix.set_expected_seq(1);
        OutPacket pkt_copy = *pkt;

        asm volatile("" ::: "memory");
        uint64_t start = rdtsc_start();

        parser_dispatch->parseFIX(&pkt_copy);

        uint64_t end = rdtsc_end();
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

    state.counters["p50_cycles"] = percentile(0.50);
    state.counters["p95_cycles"] = percentile(0.95);
    state.counters["p99_cycles"] = percentile(0.99);
}

BENCHMARK_REGISTER_F(FixBench_singlethread, Parse)->Arg(false)->Arg(true);
BENCHMARK_REGISTER_F(FixBench_multithread, Parse)->Arg(false)->Arg(true);