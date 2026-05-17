#include "dataset_network.h"
#include "dataset_builder.h"
#include "dataset_order_manager.h"
#include "TradingEngine.h"
#include "Logger.h"
#include "perf_utils.h"

#include <memory>
#include <atomic>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

static constexpr uint16_t FAKE_FIX_PORT       = 9876;
static constexpr uint16_t FAKE_OUCH_BIST_PORT = 1234;
static constexpr uint16_t FAKE_OUCH_NQ_PORT   = 4321;

struct FakeTCPServer
{
    int server_fd{-1};
    int client_fd{-1};
    uint16_t port;

    std::atomic<bool> connected{false};
    std::atomic<bool> accepted{false};
    std::thread accept_thread;

    explicit FakeTCPServer(uint16_t port) : port(port) {}

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
        });
    }

    void wait_connecting() { while (!connected.load(std::memory_order_acquire)) _mm_pause(); }
    void wait_accepting()  { while (!accepted.load(std::memory_order_acquire))  _mm_pause(); }

    void recv_from_client()
    {
        uint8_t buf[4096];
        ::recv(client_fd, buf, sizeof(buf), 0);
    }

    void stop_server()
    {
        if (client_fd >= 0) { ::shutdown(client_fd, SHUT_RDWR); ::close(client_fd); client_fd = -1; }
        if (server_fd >= 0) { ::close(server_fd); server_fd = -1; }
        if (accept_thread.joinable()) accept_thread.join();
    }
};

int main()
{
    pin_to_cpu(6);
    Logger::Init();

    auto fix_server       = std::make_unique<FakeTCPServer>(FAKE_FIX_PORT);
    auto ouch_bist_server = std::make_unique<FakeTCPServer>(FAKE_OUCH_BIST_PORT);
    auto ouch_nq_server   = std::make_unique<FakeTCPServer>(FAKE_OUCH_NQ_PORT);

    fix_server->start();
    ouch_bist_server->start();
    ouch_nq_server->start();

    auto engine = std::make_unique<TradingEngine>();

    engine->parser_to_store_.push(MessageWithVenue<MessageTypes_t>{BIST::ITCHMessage{&test_data_ordMngr::bist_dir}, Venue::BIST});
    engine->ord_mngr_.store();
    engine->parser_to_store_.push(MessageWithVenue<MessageTypes_t>{NASDAQ::ITCHMessage{&test_data_ordMngr::nasdaq_dir}, Venue::NASDAQ});
    engine->ord_mngr_.store();

    auto* symmeta        = engine->ord_mngr_.get_symbolmeta(Venue::BIST, 3);
    auto* ticksize_entry = new TickSizeEntry(0, 100'000'000, 10);
    symmeta->tick_size_table[0].store(ticksize_entry, std::memory_order_relaxed);

    auto& symRisk = engine->risk_.symbolrisks_[0][0];
    symRisk.best_bid.store(10000, std::memory_order_relaxed);
    symRisk.best_ask.store(10000, std::memory_order_relaxed);

    auto& symRisk2 = engine->risk_.symbolrisks_[1][0];
    symRisk2.best_bid.store(10000, std::memory_order_relaxed);
    symRisk2.best_ask.store(10000, std::memory_order_relaxed);

    fix_server->wait_accepting();
    ouch_bist_server->wait_accepting();
    ouch_nq_server->wait_accepting();

    engine->session_manager_.setSessionLoggedIn(0, true);
    engine->session_manager_.setSessionLoggedIn(1, true);
    engine->session_manager_.setSessionLoggedIn(2, true);

    engine->start();

    fix_server->wait_connecting();
    ouch_bist_server->wait_connecting();
    ouch_nq_server->wait_connecting();

    auto run_case = [&](int ord_case)
    {
        uint64_t before = engine->network_io_.pipeline_seq.load(std::memory_order_acquire);
        std::atomic_thread_fence(std::memory_order_seq_cst);

        switch (ord_case)
        {
            case 1: engine->risk_.strategy_to_risk_.push(test_data_builder::fix_new_order2);    break;
            case 2: engine->risk_.strategy_to_risk_.push(test_data_builder::ouch_new_order);    break;
            case 3: engine->risk_.strategy_to_risk_.push(test_data_builder::NQouch_new_order);  break;
            default: __builtin_unreachable();
        }

        while (engine->network_io_.pipeline_seq.load(std::memory_order_acquire) <= before)
            _mm_pause();

        switch (ord_case)
        {
            case 1: fix_server->recv_from_client();       break;
            case 2: ouch_bist_server->recv_from_client(); break;
            case 3: ouch_nq_server->recv_from_client();   break;
            default: __builtin_unreachable();
        }
    };

    constexpr int WARMUP = 10'000;
    for (int i = 0; i < WARMUP; i++)
        run_case((i % 3) + 1);

    constexpr int N = 5'000'000;
    for (int i = 0; i < N; i++)
        run_case((i % 3) + 1);

    engine->stop();
    engine.reset();

    fix_server->stop_server();
    ouch_bist_server->stop_server();
    ouch_nq_server->stop_server();

    Logger::Shutdown();
    return 0;
}