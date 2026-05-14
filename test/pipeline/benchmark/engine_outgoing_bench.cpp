#include <benchmark/benchmark.h>

#include "dataset.h"
#include "TradingEngine.h"

#include <memory>
#include <thread>
#include <atomic>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <queue>

// ============================================================
//                      FAKE SERVER PORTS
// ============================================================
    static constexpr uint16_t FAKE_FIX_PORT   = 9876;      // session 0 - FIX TCP
    static constexpr uint16_t FAKE_OUCH_BIST_PORT = 1234;  // session 1 - OUCH TCP
    static constexpr uint16_t FAKE_OUCH_NQ_PORT   = 4321;  // session 2 - OUCH TCP
    static constexpr uint16_t FAKE_ITCH_BIST_PORT = 5000;  // session 3 - ITCH UDP
    static constexpr uint16_t FAKE_ITCH_NQ_PORT   = 6000;  // session 4 - ITCH UDP

// ============================================================
//                      FAKE TCP SERVER
// ============================================================
    struct FakeTCPServer
    {
        int server_fd{-1};
        int client_fd{-1};
        uint16_t port;
        
        std::atomic<bool> stop{false};

        std::atomic<bool> listening{false};
        std::atomic<bool> accepted{false};
        std::atomic<bool> connected{false};
        
        std::thread accept_thread;
        std::thread recv_thread;
        std::thread send_thread;

        std::vector<std::vector<uint8_t>> received_data;
        std::atomic<bool> received{false};

        std::queue<std::vector<uint8_t>> send_queue;
        std::atomic<bool> sent{false};
    
        FakeTCPServer(uint16_t port) : port(port) {}
        
        void start()
        {
            server_fd = ::socket(AF_INET, SOCK_STREAM, 0);
            int reuse = 1;
            ::setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
            ::setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));

            sockaddr_in addr{};
            addr.sin_family      = AF_INET;
            addr.sin_port        = htons(port);
            addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            ::bind(server_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
            ::listen(server_fd, 5);
        
            listening.store(true, std::memory_order_release);
        
            accept_thread = std::thread([this]() {
       
                sockaddr_in client_addr{};
                socklen_t len = sizeof(client_addr);
                
                accepted.store(true, std::memory_order_release);

                while (true) {
                    client_fd = ::accept(server_fd,
                                        reinterpret_cast<sockaddr*>(&client_addr),
                                        &len);
                    
                    if (client_fd >= 0) break;
                    if (errno == EINTR) continue;
                    break;  
                }
                
                if(client_fd >= 0) 
                    connected.store(true, std::memory_order_release);
                if(client_fd < 0) return;  
                
                recv_thread = std::thread([this]() { 
                    uint8_t buf[4096];
                    while (!stop.load()) 
                    {
                        ssize_t n = ::recv(client_fd, buf, sizeof(buf), 0);
                        if (n > 0) {
                            {
                                std::lock_guard<std::mutex> lk(recv_mutex);
                                received_data.push_back(std::vector<uint8_t>(buf, buf + n));
                            }
                            recv_cv.notify_all();
                        }
                        else if (n == 0) {
                            break;  
                        }
                        else {
                            if (errno == EINTR) continue; 
                            break;  
                        }
                    }
                });


                send_thread = std::thread([this]() {
                    while (!stop.load()) 
                    {
                        std::unique_lock<std::mutex> lk(send_mutex);
                        send_cv.wait_for(lk, std::chrono::milliseconds(10), [this]() {
                            return !send_queue.empty() || stop.load();
                        });

                        while (!send_queue.empty()) {
                            auto& msg = send_queue.front();
                            ::send(client_fd, msg.data(), msg.size(), 0);
                            send_queue.pop();    
                        }

                        states_ready_cv.notify_all();
                        packets_ready_cv.notify_all();
                    }
                });

            });
        }

        void send_to_client(const auto& msg)
        {
            {
                std::lock_guard<std::mutex> lk(send_mutex);
                send_queue.push(msg);
            }

            send_cv.notify_all();
        }

        void stop_server() {
            stop.store(true);
            send_cv.notify_all();

            if (client_fd >= 0) {
                ::shutdown(client_fd, SHUT_RDWR); 
                ::close(client_fd);
                client_fd = -1;
            }
            if (server_fd >= 0) {
                ::close(server_fd);  
                server_fd = -1;
            }

            if (recv_thread.joinable())   recv_thread.join();
            if (send_thread.joinable())   send_thread.join();
            if (accept_thread.joinable()) accept_thread.join();
           
        }
    };

// ============================================================
//                      FAKE UDP SENDER
// ============================================================
    struct FakeUDPSender
    {
        int sock_fd{-1};
        uint16_t port;

        FakeUDPSender(uint16_t port) : port(port)
        {
            sock_fd = ::socket(AF_INET, SOCK_DGRAM, 0);
        }

        void send_to(const auto& msg)
        {
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            ::sendto(sock_fd, msg.data(), msg.size(), 0, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
        }

        ~FakeUDPSender()
        {
            if (sock_fd >= 0) ::close(sock_fd);
        }
    };



class BM_TradingEngine : public benchmark::Fixture
{
public:
    std::unique_ptr<TradingEngine> engine;

    SessionState* sess_state;
    OutPacket* pkt;
    std::vector<OutPacket*> pkts;
    int pkt_case;

    FakeTCPServer fix_server(FAKE_FIX_PORT);
    FakeTCPServer ouch_bist_server(FAKE_OUCH_BIST_PORT);
    FakeTCPServer ouch_nq_server(FAKE_OUCH_NQ_PORT);
    FakeUDPSender itch_bist_sender(FAKE_ITCH_BIST_PORT);
    FakeUDPSender itch_nq_sender(FAKE_ITCH_NQ_PORT);

public:
    void SetUp(const ::benchmark::State& st) 
    {
        pkts.clear(); 

        engine     = std::make_unique<TradingEngine>();

        fix_server.start();
        ouch_bist_server.start();
        ouch_nq_server.start();
        
        uint8_t sess_index = sess_mngr->getSessionIndex(Venue::BIST, Protocol::FIX);
        sess_state = sess_mngr->getSessionState(sess_index);
        
        pkt_case = st.range(0);
        switch(pkt_case)
        {
            case 1: 
                pkts.push_back(&test_data::fix_order_ack); 
                break;
            case 2: 
                pkts.push_back(&test_data::ouch_bist_order_ack); 
                break;
            case 3: 
                pkts.push_back(&test_data::ouch_nq_order_ack);
                break;
            case 4: 
                pkts.push_back(&test_data::itch_bist_add_order); 
                break;
            case 5: 
                pkts.push_back(&test_data::itch_nq_add_order);
                break;
            default: 
                __builtin_unreachable(); 
        }  
    }

    void TearDown(const ::benchmark::State& st) 
    {
        engine.reset();
    }
};


BENCHMARK_DEFINE_F(BM_Parser, ParseFIX)(benchmark::State& state)
{
    pin_to_cpu(6);

    std::vector<uint64_t> latencies;
    latencies.reserve(100000);
    
    for(auto _ : state)
    {
        sess_state->fix.set_expected_seq(1);
        
        asm volatile("" ::: "memory");
        auto wall_start = std::chrono::high_resolution_clock::now();
        uint64_t start = rdtsc_start();

        // for (auto* pkt : pkts)
        //     parser_dispatch->parseFIX(pkt);

        uint64_t end = rdtsc_end();
        auto wall_end = std::chrono::high_resolution_clock::now();
        
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(wall_end - wall_start).count();
        state.SetIterationTime(elapsed * 1e-9);

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

BENCHMARK_REGISTER_F(BM_Parser, ParseFIX)->Arg(0)->UseManualTime()->Name("BM_ParserFIX_best");
BENCHMARK_REGISTER_F(BM_Parser, ParseFIX)->Arg(1)->UseManualTime()->Name("BM_ParserFIX_normal");
BENCHMARK_REGISTER_F(BM_Parser, ParseFIX)->Arg(2)->UseManualTime()->Name("BM_ParserFIX_worst(seq-in-order)");
BENCHMARK_REGISTER_F(BM_Parser, ParseFIX)->Arg(3)->UseManualTime()->Name("BM_ParserFIX_worst(seq-out-of-order)");




















#include <benchmark/benchmark.h>

#include "Parser_Dispatch.h"
#include "SessionManager.h"
#include "LoginController.h"
#include "SoupBinTcp.h"
#include "Builder_FIX.h"
#include "NetworkIO.h"
#include "Parser_FIX.h"
#include "dataset.h"
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

    std::atomic<bool> stop{false};
    std::thread consumer;

public:
    void SetUp(const ::benchmark::State& st) 
    {
        stop.store(false, std::memory_order_relaxed);
        pkts.clear(); 

        engine     = std::make_unique<TradingEngine>();
        
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
                                        *inPkt_pool
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
                pkts.push_back(&test_data::fix_outpacket_single_1); // 1msg 1pkt
                break;
            case 2: 
                pkts.push_back(&test_data::fix_outpacket_full_1); // 3msgs 1pkt
                break;
            case 3: 
                pkts.push_back(&test_data::fix_outpacket_partial_1); // 3msgs scattered 3pkts, in order
                pkts.push_back(&test_data::fix_outpacket_partial_2);
                pkts.push_back(&test_data::fix_outpacket_partial_3); 
                break;
            case 4: 
                pkts.push_back(&test_data::fix_outpacket_partial_4); // 3msgs scattered 3pkts, out of order
                pkts.push_back(&test_data::fix_outpacket_partial_5);
                pkts.push_back(&test_data::fix_outpacket_partial_6); 
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







  

    

  


  


#include <benchmark/benchmark.h>

#include "OrderManager.h"
#include "MarketBook.h"
#include "HashTables.h"
#include "Order.h"
#include "dataset.h"
#include "RiskEngine.h"
#include "Limits.h"
#include "Logger.h"
#include "bench_utils.h"

#include <thread>
#include <atomic>
#include <vector>
#include <array>
#include <memory>
#include <chrono>

inline void reset_accountrisk(AccountRisk& ar) noexcept;
inline void reset_symbolrisk(SymbolRisk& sr) noexcept;

class BM_Risk : public benchmark::Fixture
{
public:
    std::unique_ptr<HashTables> hashtables;
    std::unique_ptr<Limits> limits;
    std::unique_ptr<MarketBook> marketbook;
    std::unique_ptr<OrderManager> order_manager;
    std::unique_ptr<RiskEngine> risk;

    spscMessageQueue_t parser_to_store;
    spscOrderQueue_t strategy_to_risk;
    spscRejectOrderQueue_t risk_to_strategy;
    spscOrderQueue_t risk_to_builder;
    spscOrderQueue_t store_to_strategy_free_slot;
    spscOrderQueue_t store_to_risk;
    spscOrderQueue_t store_to_strategy;
    spscDbQueue_t store_to_db;

    std::atomic<bool> stop{false};
    std::thread consumer;

    std::atomic<bool> stop2{false};
    std::thread consumer2;

public:
    void SetUp(const ::benchmark::State& st) 
    {
        stop.store(false, std::memory_order_release);
        stop2.store(false, std::memory_order_release);
        
        hashtables    = std::make_unique<HashTables>();
        limits        = std::make_unique<Limits>(*hashtables);
        marketbook    = std::make_unique<MarketBook>(*hashtables);
    
        order_manager = std::make_unique<OrderManager>(
                                            parser_to_store,
                                            store_to_strategy,
                                            store_to_strategy_free_slot,
                                            store_to_risk,
                                            store_to_db,
                                            *marketbook,
                                            *hashtables        
        );

        risk          = std::make_unique<RiskEngine>(
                                        store_to_risk,
                                        strategy_to_risk,
                                        risk_to_strategy,
                                        risk_to_builder,
                                        *hashtables,
                                        *marketbook,
                                        *limits,
                                        *order_manager   

        );

        //SYMBOLMETA INITIALIZING  (BIST GARAN(order_book_id = 3) and NASDAQ AAPL(stock_locate = 1))
        parser_to_store.push(MessageWithVenue<MessageTypes_t>{NASDAQ::ITCHMessage{&test_data::nasdaq_dir}, Venue::NASDAQ});
        order_manager->store();
        
        parser_to_store.push(MessageWithVenue<MessageTypes_t>{BIST::ITCHMessage{&test_data::bist_dir}, Venue::BIST});
        order_manager->store();

        //TICK SIZE INITIALIZATION VENUE::BIST--0 SYMBOL::GARAN--0 TICKSIZE--10
        auto* symmeta = order_manager->get_symbolmeta(Venue::BIST, 3);
        auto* ticksize_entry = new TickSizeEntry(0, 100'000'000, 10);
        symmeta->tick_size_table[0].store(ticksize_entry, std::memory_order_relaxed);

        // BEST BID/ASK INITIALIZATION VENUE::BIST--0 SYMBOL::GARAN--0
        auto& symRisk = risk->symbolrisks_[0][0];
        symRisk.best_bid.store(10000, std::memory_order_relaxed);
        symRisk.best_ask.store(10000, std::memory_order_relaxed);

        consumer = std::thread([&]
        {
            pin_to_cpu(0);        

            Order* order; 
            
            while(!stop.load(std::memory_order_acquire))
            {
                if(!risk_to_builder.pop(order)) 
                {
                    _mm_pause();  
                }
            }
        });

        consumer2 = std::thread([&]
        {
            pin_to_cpu(4);        

            OrderWithRejectReason orderWRR; 
            
            while(!stop2.load(std::memory_order_acquire))
            {
                if(!risk_to_strategy.pop(orderWRR)) 
                    _mm_pause();  
            }
        });
    }

    void TearDown(const ::benchmark::State& st) 
    {
        stop.store(true, std::memory_order_release);
        stop2.store(true, std::memory_order_release);
        consumer.join();
        consumer2.join();

        order_manager.reset();
        marketbook.reset();
        limits.reset();
        hashtables.reset();
    }
};

//========================================================
//                  RISK UPDATE BENCHMARK
//========================================================
    BENCHMARK_DEFINE_F(BM_Risk, Update)(benchmark::State& state)
    {
        pin_to_cpu(2);
        
        int order_case = state.range(0);

        std::vector<uint64_t> latencies;
        latencies.reserve(100000);

        for(auto _ : state)
        {
            if(order_case == 0) 
            { 
                store_to_risk.push(test_data::fixNew);
            }
            else
            {
                store_to_risk.push(test_data::fixNew);
                store_to_risk.push(test_data::fixPartial);
                store_to_risk.push(test_data::fixCancel);
            }
        

            bool processing = true;

            asm volatile("" ::: "memory");
            auto wall_start = std::chrono::high_resolution_clock::now();
            uint64_t start = rdtsc_start();

            while (processing)
                processing = risk->update_risk();

            uint64_t end = rdtsc_end();
            auto wall_end = std::chrono::high_resolution_clock::now();
            
            auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(wall_end - wall_start).count();
            state.SetIterationTime(elapsed * 1e-9);
            
            benchmark::ClobberMemory();
            latencies.push_back(end - start);

            reset_accountrisk(risk->accountrisks_[0]);
            reset_symbolrisk(risk->symbolrisks_[0][0]);
            risk->orderrisks_[0].clear();
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

//========================================================
//                  RISK CHECK BENCHMARK
//========================================================
    BENCHMARK_DEFINE_F(BM_Risk, Check)(benchmark::State& state)
    {
        pin_to_cpu(2);

        std::vector<uint64_t> latencies;
        latencies.reserve(100000);

        for(auto _ : state)
        {
            strategy_to_risk.push(test_data::o1);
            bool processing = true;
            
            asm volatile("" ::: "memory");
            auto wall_start = std::chrono::high_resolution_clock::now();
            uint64_t start = rdtsc_start();

            while (processing)
                processing = risk->check_risk();

            uint64_t end = rdtsc_end();
            auto wall_end = std::chrono::high_resolution_clock::now();
            
            auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(wall_end - wall_start).count();
            state.SetIterationTime(elapsed * 1e-9);
            
            benchmark::ClobberMemory();
            latencies.push_back(end - start);
            
            reset_accountrisk(risk->accountrisks_[0]);
            reset_symbolrisk(risk->symbolrisks_[0][0]);
            risk->orderrisks_[0].clear();
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


BENCHMARK_REGISTER_F(BM_Risk, Update)->Arg(0)->UseManualTime()->Name("BM_Risk_Update_single");
BENCHMARK_REGISTER_F(BM_Risk, Update)->Arg(1)->UseManualTime()->Name("BM_Risk_Update_flow");
BENCHMARK_REGISTER_F(BM_Risk, Check)->UseManualTime()->Name("BM_Risk_Check_single");


inline void reset_accountrisk(AccountRisk& ar) noexcept
{
    ar.current_exposure.store(0, std::memory_order_relaxed);
    ar.balance.store(10'000'000, std::memory_order_relaxed);
    ar.positional_exposure.store(0, std::memory_order_relaxed);
    ar.used_margin.store(0, std::memory_order_relaxed);
    ar.current_leverage.store(1, std::memory_order_relaxed);
    ar.daily_realized_pnl.store(0, std::memory_order_relaxed);
    ar.total_unrealized_pnl.store(0, std::memory_order_relaxed);
    ar.open_orders_count.store(0, std::memory_order_relaxed);
}

inline void reset_symbolrisk(SymbolRisk& sr) noexcept
{
    sr.net_position.store(0, std::memory_order_relaxed);
    sr.cost_basis_scaled.store(0, std::memory_order_relaxed);
    sr.unrealized_pnl.store(0, std::memory_order_relaxed);
    sr.realized_pnl.store(0, std::memory_order_relaxed);
    sr.pending_notional_scaled.store(0, std::memory_order_relaxed);
    sr.best_bid.store(10000, std::memory_order_relaxed);
    sr.best_ask.store(10000, std::memory_order_relaxed);
    sr.avg_entry_price.store(0, std::memory_order_relaxed);
    sr.open_orders_count.store(0, std::memory_order_relaxed);
}




#include <benchmark/benchmark.h>

#include "OrderManager.h"
#include "MarketBook.h"
#include "HashTables.h"
#include "Order.h"
#include "dataset.h"
#include "bench_utils.h"

#include <thread>
#include <atomic>
#include <vector>
#include <array>
#include <memory>
#include <chrono>

class BM_OrderManager : public benchmark::Fixture
{
public:
    std::unique_ptr<HashTables> hashtables;
    std::unique_ptr<MarketBook> marketbook;
    std::unique_ptr<OrderManager> order_manager;

    spscMessageQueue_t parser_to_store;
    spscOrderQueue_t store_to_strategy;
    spscOrderQueue_t store_to_strategy_free_slot;
    spscOrderQueue_t store_to_risk;
    spscDbQueue_t store_to_db;

    std::atomic<bool> stop{false};
    std::thread consumer;

    std::atomic<bool> stop2{false};
    std::thread consumer2;

    std::atomic<bool> stop3{false};
    std::thread consumer3;

    std::array<MessageWithVenue<MessageTypes_t>, 17> ord_manager_traffic;

public:
    
    void SetUp(const ::benchmark::State& st) 
    {
        stop.store(false, std::memory_order_release);
        stop2.store(false, std::memory_order_release);
        stop3.store(false, std::memory_order_release);
        
        Order* order;
        while(store_to_strategy_free_slot.pop(order)) {}

        hashtables    = std::make_unique<HashTables>();
        marketbook    = std::make_unique<MarketBook>(*hashtables);
    
        order_manager = std::make_unique<OrderManager>(
                                            parser_to_store,
                                            store_to_strategy,
                                            store_to_strategy_free_slot,
                                            store_to_risk,
                                            store_to_db,
                                            *marketbook,
                                            *hashtables        
        );

        ord_manager_traffic = []()
        {
            std::array<MessageWithVenue<MessageTypes_t>, 17> arr{};

            // 0 — NASDAQ ITCH StockDirectory (AAPL)
            arr[0] = MessageWithVenue<MessageTypes_t>{NASDAQ::ITCHMessage{&test_data::nasdaq_dir}, Venue::NASDAQ};

            // 1 — BIST ITCH OrderBookDirectory (GARAN)
            arr[1] = MessageWithVenue<MessageTypes_t>{BIST::ITCHMessage{&test_data::bist_dir}, Venue::BIST}; 

            // 2 — FIX New Order (GARAN, buy 500@15000)
            arr[2] = MessageWithVenue<MessageTypes_t>{&test_data::fix_new, Venue::BIST};  /////////////

            // 3 — BIST OUCH OrderAccepted (GARAN, order_id=42)
            arr[3] = MessageWithVenue<MessageTypes_t>{BIST::OUCHMessage{&test_data::bist_acc}, Venue::BIST};  ///////////////

            // 4 — BIST ITCH AddOrder (GARAN, 1000@10000)
            arr[4] = MessageWithVenue<MessageTypes_t>{BIST::ITCHMessage{&test_data::bist_add}, Venue::BIST}; //////////////

            // 5 — NASDAQ OUCH OrderAccepted (AAPL, user_ref=99)
            arr[5] = MessageWithVenue<MessageTypes_t>{NASDAQ::OUCHMessage{&test_data::nasdaq_acc}, Venue::NASDAQ};   //////////////////

            // 6 — NASDAQ ITCH AddOrder (AAPL, 100@10000)
            arr[6] = MessageWithVenue<MessageTypes_t>{NASDAQ::ITCHMessage{&test_data::nasdaq_add}, Venue::NASDAQ};  /////////////

            // 7 — FIX Partial Fill (GARAN, 200 filled)
            arr[7] = MessageWithVenue<MessageTypes_t>{&test_data::fix_partial, Venue::BIST}; ////////////////

            // 8 — BIST ITCH OrderExecuted (GARAN, 60 executed)
            arr[8] = MessageWithVenue<MessageTypes_t>{BIST::ITCHMessage{&test_data::bist_exec}, Venue::BIST}; ////////////////

            // 9 — BIST OUCH OrderExecuted (GARAN, 600 traded)
            arr[9] = MessageWithVenue<MessageTypes_t>{BIST::OUCHMessage{&test_data::bist_execO}, Venue::BIST}; //////////

            // 10 — NASDAQ ITCH OrderExecuted (AAPL, 60 executed)
            arr[10] = MessageWithVenue<MessageTypes_t>{NASDAQ::ITCHMessage{&test_data::nasdaq_exec}, Venue::NASDAQ}; 

            // 11 — NASDAQ OUCH OrderExecuted (AAPL, 600 executed)
            arr[11] = MessageWithVenue<MessageTypes_t>{NASDAQ::OUCHMessage{&test_data::nasdaq_execO}, Venue::NASDAQ};   //////////////

            // 12 — BIST ITCH OrderDelete (GARAN, kalan 40 deleted)
            arr[12] = MessageWithVenue<MessageTypes_t>{BIST::ITCHMessage{&test_data::bist_cancel}, Venue::BIST}; ////////////////

            // 13 — BIST OUCH OrderCancelled (GARAN)
            arr[13] = MessageWithVenue<MessageTypes_t>{BIST::OUCHMessage{&test_data::bist_canO}, Venue::BIST}; /////////////

            // 14 — NASDAQ ITCH OrderDelete (AAPL, remaining 40 deleted)
            arr[14] = MessageWithVenue<MessageTypes_t>{NASDAQ::ITCHMessage{&test_data::nasdaq_delete}, Venue::NASDAQ};

            // 15 — NASDAQ OUCH OrderCancelled (AAPL)
            arr[15] = MessageWithVenue<MessageTypes_t>{NASDAQ::OUCHMessage{&test_data::nasdaq_canO}, Venue::NASDAQ};  //////////////////

            // 16 — FIX Cancel Confirm (GARAN)
            arr[16] = MessageWithVenue<MessageTypes_t>{&test_data::fix_cancel, Venue::BIST}; ///////////////
        
            return arr;
        }();
        
        for(size_t i = 0; i < 2; i++)
        {
            parser_to_store.push(ord_manager_traffic[i]);
            order_manager->store();
        }

        consumer = std::thread([&]
        {
            pin_to_cpu(0);        

            Order* order; 
            
            while(!stop.load(std::memory_order_acquire))
            {
                if(!store_to_risk.pop(order)) 
                {
                    _mm_pause();  
                }
            }
        });

        consumer2 = std::thread([&]
        {
            pin_to_cpu(4);        

            Order* order; 
            
            while(!stop2.load(std::memory_order_acquire))
            {
                if(!store_to_strategy.pop(order)) 
                {
                    _mm_pause();  
                }
            }
        });

        consumer3 = std::thread([&]
        {
            pin_to_cpu(6);        

            Order* order; 
            
            DbData_t DbData; 

            while(!stop3.load(std::memory_order_acquire))
            {
                if(!store_to_db.pop(DbData)) 
                {
                    _mm_pause();  
                }
            }
        });
    }

    void refresh_awaiting_map_per_ite()
    {
        // BIST FIX order
        {
            Order* order = nullptr;
            store_to_strategy_free_slot.pop(order);

            order->price              = test_data::fix_new.price;
            order->quantity           = test_data::fix_new.quantity;
            order->side               = Side::Buy;
            order->venue              = Venue::BIST;
            order->isOurOrder         = true;
            order->protocol           = Protocol::FIX;
            order->instrument_id      = 3;
            order->order_type         = static_cast<OrderType>(test_data::fix_new.ord_type);
            order->symbol             = {"GARAN"};
            std::strncpy(order->client_order_token.data(), test_data::fix_new.cl_ord_id, ORDER_TOKEN_SIZE);
            order->client_order_id    = absl::Hash<std::string_view>{}(test_data::fix_new.cl_ord_id);

            order_manager->add_awaitingAck_order(*order);
        }

        // BIST OUCH order
        {
            Order* order = nullptr;
            store_to_strategy_free_slot.pop(order);

            order->price              = test_data::bist_acc.price;
            order->quantity           = test_data::bist_acc.quantity;
            order->side               = Side::Buy;
            order->venue              = Venue::BIST;
            order->isOurOrder         = true;
            order->protocol           = Protocol::OUCH;
            order->instrument_id      = test_data::bist_acc.order_book_id;
            order->symbol             = {"GARAN"};
            std::strncpy(order->client_order_token.data(), test_data::bist_acc.order_token, sizeof(test_data::bist_acc.order_token));
            order->client_order_id    = absl::Hash<std::string_view>{}(std::string_view{test_data::bist_acc.order_token, 14});

            order_manager->add_awaitingAck_order(*order);
        }

        // NASDAQ OUCH order
        {
            Order* order = nullptr;
            store_to_strategy_free_slot.pop(order);

            order->price              = test_data::nasdaq_acc.price;
            order->quantity           = test_data::nasdaq_acc.quantity;
            order->side               = Side::Buy;
            order->venue              = Venue::NASDAQ;
            order->isOurOrder         = true;
            order->protocol           = Protocol::OUCH;
            order->user_ref_num       = test_data::nasdaq_acc.user_ref_num;
            order->symbol             = {"AAPL"};
            order->client_order_id    = absl::Hash<std::string_view>{}(test_data::nasdaq_acc.cl_ord_id);
            std::strncpy(order->client_order_token.data(), test_data::nasdaq_acc.cl_ord_id, sizeof(test_data::nasdaq_acc.cl_ord_id));

            order_manager->add_awaitingAck_order(*order);
        }
    }

    void TearDown(const ::benchmark::State& st) 
    {
        stop.store(true, std::memory_order_release);
        stop2.store(true, std::memory_order_release);
        stop3.store(true, std::memory_order_release);
        consumer.join();
        consumer2.join();
        consumer3.join();

        order_manager.reset();
        marketbook.reset();
        hashtables.reset();

    }
};

BENCHMARK_DEFINE_F(BM_OrderManager, MixedTraffic)(benchmark::State& state)
{
    pin_to_cpu(2);

    std::vector<uint64_t> latencies;
    latencies.reserve(100000);

    for(auto _ : state)
    {
        for(size_t i = 2; i < 17; i++)
            parser_to_store.push(ord_manager_traffic[i]);

        bool processing = true;
        refresh_awaiting_map_per_ite();

        asm volatile("" ::: "memory");
        auto wall_start = std::chrono::high_resolution_clock::now();
        uint64_t start = rdtsc_start();

        while (processing)
            processing = order_manager->store();

        uint64_t end = rdtsc_end();
        auto wall_end = std::chrono::high_resolution_clock::now();
        
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(wall_end - wall_start).count();
        state.SetIterationTime(elapsed * 1e-9);
        
        benchmark::ClobberMemory();
        latencies.push_back(end - start);

        for(auto& [key, ord] : order_manager->our_orders_)
            store_to_strategy_free_slot.push(ord);
        order_manager->our_orders_.clear();
    
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

BENCHMARK_REGISTER_F(BM_OrderManager, MixedTraffic)->UseManualTime();

   
  
    

