#include <benchmark/benchmark.h>

#include "dataset_network.h"
#include "dataset_builder.h"
#include "dataset_order_manager.h"
#include "TradingEngine.h"
#include "bench_utils.h"
#include "Logger.h"

#include <memory>
#include <atomic>
#include <chrono>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

static constexpr uint16_t FAKE_FIX_PORT       = 9876;
static constexpr uint16_t FAKE_OUCH_BIST_PORT = 1234;
static constexpr uint16_t FAKE_OUCH_NQ_PORT   = 4321;
static constexpr uint16_t FAKE_ITCH_BIST_PORT = 5000;
static constexpr uint16_t FAKE_ITCH_NQ_PORT   = 6000;

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

struct FakeTCPServer
{
    int server_fd{-1};
    int client_fd{-1};
    uint16_t port;

    // std::atomic<bool> stop{false};
    // bool listening{false};
    std::atomic<bool> connected{false};
    std::atomic<bool> accepted{false};

    std::thread accept_thread;

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

        // // listening.store(true, std::memory_order_relaxed); 
        // listening = true;
        
        accept_thread = std::thread([this]()
        {
            pin_to_cpu(15);

            sockaddr_in client_addr{};
            socklen_t len = sizeof(client_addr);

            accepted.store(true, std::memory_order_seq_cst);

            while (true)
            {
                client_fd = ::accept(server_fd, reinterpret_cast<sockaddr*>(&client_addr), &len);
                if (client_fd >= 0) break;
                if (errno == EINTR) continue;
                break;
            }

            if (client_fd >= 0)
                connected.store(true, std::memory_order_seq_cst);

            std::cerr << "connected oldu: " << port << "\n";
        });
    }

    // void wait_listening()
    // {
    //     while (!listening)
    //         _mm_pause();
    // }

    void wait_connecting()
    {
        while (!connected.load(std::memory_order_acquire))
            _mm_pause();
    }
    
    void wait_accepting() 
    {
        while (!accepted.load(std::memory_order_acquire))
            _mm_pause();
    }

    void drain_login()
    {
        uint8_t buf[4096];
        ssize_t n = ::recv(client_fd, buf, sizeof(buf), 0);
        (void)n;
    }

    void send_to_client(const auto& msg)
    {
        ::send(client_fd, msg.data(), msg.size(), 0);
    }

    void stop_server()
    {
        // stop.store(true);
        if (client_fd >= 0) { ::shutdown(client_fd, SHUT_RDWR); ::close(client_fd); client_fd = -1; }
        if (server_fd >= 0) { ::close(server_fd); server_fd = -1; }
        if (accept_thread.joinable()) accept_thread.join();
    }
};

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
        addr.sin_family      = AF_INET;
        addr.sin_port        = htons(port);
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        ::sendto(sock_fd, msg.data(), msg.size(), 0, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    }

    ~FakeUDPSender()
    {
        if (sock_fd >= 0) ::close(sock_fd);
    }
};

class BM_Pipeline_Incoming : public benchmark::Fixture
{
public:
    std::unique_ptr<TradingEngine> engine;    
    std::unique_ptr<FakeTCPServer> fix_server;
    std::unique_ptr<FakeTCPServer> ouch_bist_server;
    std::unique_ptr<FakeTCPServer> ouch_nq_server;

    FakeUDPSender itch_bist_sender{FAKE_ITCH_BIST_PORT};
    FakeUDPSender itch_nq_sender{FAKE_ITCH_NQ_PORT};

    int pkt_case;
    uint8_t venue_index;

public:
    void SetUp(const ::benchmark::State& st)
    {
        Logger::Init();

        fix_server       = std::make_unique<FakeTCPServer>(FAKE_FIX_PORT);
        ouch_bist_server = std::make_unique<FakeTCPServer>(FAKE_OUCH_BIST_PORT);
        ouch_nq_server   = std::make_unique<FakeTCPServer>(FAKE_OUCH_NQ_PORT);

        fix_server->start();
        ouch_bist_server->start();
        ouch_nq_server->start();

        engine = std::make_unique<TradingEngine>();

        engine->parser_to_store_.push(MessageWithVenue<MessageTypes_t>{NASDAQ::ITCHMessage{&test_data_ordMngr::nasdaq_dir}, Venue::NASDAQ});
        engine->ord_mngr_.store();
        engine->parser_to_store_.push(MessageWithVenue<MessageTypes_t>{BIST::ITCHMessage{&test_data_ordMngr::bist_dir}, Venue::BIST});
        engine->ord_mngr_.store();

        engine->risk_.pipeline_seq.store(0, std::memory_order_relaxed);
        
        std::cerr << "accept bekleniyor ";
        fix_server->wait_accepting();
        ouch_bist_server->wait_accepting();
        ouch_nq_server->wait_accepting();
        
        engine->start();
        std::cerr << "accept oldu connect bekleniyor ";
        fix_server->wait_connecting();
        ouch_bist_server->wait_connecting();
        ouch_nq_server->wait_connecting();
        std::cerr << "connected oldu mainden devam ";
        pkt_case = st.range(0);
        venue_index = (pkt_case == 5 || pkt_case == 3) ? static_cast<uint8_t>(Venue::NASDAQ)
                                                        : static_cast<uint8_t>(Venue::BIST);
    }

    void TearDown(const ::benchmark::State& st)
    {
        std::cerr << "STOP BAŞLADI\n";
        engine->stop();
        std::cerr << "ENGINE STOP BİTTİ\n";
        engine.reset();
        std::cerr << "ENGINE RESET BİTTİ\n";
        fix_server->stop_server();
        ouch_bist_server->stop_server();
        ouch_nq_server->stop_server();
        fix_server.reset();
        ouch_bist_server.reset();
        ouch_nq_server.reset();
        Logger::Shutdown();
        std::cerr << "STOP BİTTİ\n";
    }
        
};

BENCHMARK_DEFINE_F(BM_Pipeline_Incoming, PerProtocol)(benchmark::State& state)
{
    pin_to_cpu(6);

    std::vector<uint64_t> latencies;
    latencies.reserve(100000);

    for (auto _ : state)
    {
        uint64_t before = engine->risk_.pipeline_seq.load(std::memory_order_acquire);

        std::cerr << "before: " << before << " ";

        engine->ord_mngr_.add_awaitingAck_order(*test_data_builder::fix_new_order);
        engine->ord_mngr_.add_awaitingAck_order(*test_data_builder::ouch_new_order);
        engine->ord_mngr_.add_awaitingAck_order(*test_data_builder::NQouch_new_order);


        switch (pkt_case)
        {
            case 1: fix_server->send_to_client(test_data_network::fix_order_ack);             break;
            case 2: ouch_bist_server->send_to_client(test_data_network::ouch_bist_order_ack); break;
            case 3: ouch_nq_server->send_to_client(test_data_network::ouch_nq_order_ack);     break;
            case 4: itch_bist_sender.send_to(test_data_network::itch_bist_add_order2);        break;
            case 5: itch_nq_sender.send_to(test_data_network::itch_nq_add_order);            break;
            default: __builtin_unreachable();
        }

        asm volatile("" ::: "memory");
        auto wall_start = std::chrono::high_resolution_clock::now();
        uint64_t start  = rdtsc_start();

        while (engine->risk_.pipeline_seq.load(std::memory_order_acquire) <= before)
            _mm_pause();
       
        uint64_t end  = rdtsc_end();
        auto wall_end = std::chrono::high_resolution_clock::now();

        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(wall_end - wall_start).count();
        state.SetIterationTime(elapsed * 1e-9);

        benchmark::ClobberMemory();
        latencies.push_back(end - start);

        std::cerr << "benchmark bir kez  tamamlandı: " << engine->risk_.pipeline_seq.load(std::memory_order_acquire) << " ";
        reset_accountrisk(engine->risk_.accountrisks_[venue_index]);
        reset_symbolrisk(engine->risk_.symbolrisks_[venue_index][0]);
        engine->risk_.orderrisks_[venue_index].clear();

        engine->hashtables_.exec_id_hashes_.fill(0);

        engine->ord_mngr_.our_orders_.clear();
        engine->ord_mngr_.our_orders_wtokenkey_.clear();
        engine->ord_mngr_.awaitingAck_orders_.clear();
        engine->ord_mngr_.nq_itch_refnum_ordkey_.clear();
        engine->ord_mngr_.market_orders_.clear();

        engine->session_manager_.getSessionState(0)->fix.set_expected_seq(1);
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

// BENCHMARK_REGISTER_F(BM_Pipeline_Incoming, PerProtocol)->Arg(1)->ArgName("FIX")->UseManualTime();
// BENCHMARK_REGISTER_F(BM_Pipeline_Incoming, PerProtocol)->Arg(2)->ArgName("OUCH_BIST")->UseManualTime();
// BENCHMARK_REGISTER_F(BM_Pipeline_Incoming, PerProtocol)->Arg(3)->ArgName("OUCH_NQ")->UseManualTime();
// BENCHMARK_REGISTER_F(BM_Pipeline_Incoming, PerProtocol)->Arg(4)->ArgName("ITCH_BIST")->UseManualTime();
// BENCHMARK_REGISTER_F(BM_Pipeline_Incoming, PerProtocol)->Arg(5)->ArgName("ITCH_NQ")->UseManualTime();
