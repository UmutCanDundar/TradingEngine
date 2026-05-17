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
static constexpr uint16_t FAKE_ITCH_BIST_PORT = 5000;
static constexpr uint16_t FAKE_ITCH_NQ_PORT   = 6000;

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

    void send_to_client(const auto& msg)
    {
        ::send(client_fd, msg.data(), msg.size(), 0);
    }

    void stop_server()
    {
        if (client_fd >= 0) { ::shutdown(client_fd, SHUT_RDWR); ::close(client_fd); client_fd = -1; }
        if (server_fd >= 0) { ::close(server_fd); server_fd = -1; }
        if (accept_thread.joinable()) accept_thread.join();
    }
};

struct FakeUDPSender
{
    int sock_fd{-1};
    uint16_t port;

    explicit FakeUDPSender(uint16_t port) : port(port)
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

    ~FakeUDPSender() { if (sock_fd >= 0) ::close(sock_fd); }
};

int main()
{
    pin_to_cpu(6);
    Logger::Init();

    auto fix_server       = std::make_unique<FakeTCPServer>(FAKE_FIX_PORT);
    auto ouch_bist_server = std::make_unique<FakeTCPServer>(FAKE_OUCH_BIST_PORT);
    auto ouch_nq_server   = std::make_unique<FakeTCPServer>(FAKE_OUCH_NQ_PORT);

    FakeUDPSender itch_bist_sender{FAKE_ITCH_BIST_PORT};
    FakeUDPSender itch_nq_sender{FAKE_ITCH_NQ_PORT};

    fix_server->start();
    ouch_bist_server->start();
    ouch_nq_server->start();

    auto engine = std::make_unique<TradingEngine>();

    BIST::ITCHOrderBookDirectoryMessage *m = nullptr;
    engine->parser_dispatch_.itchparser_bist_.itch_pools_.get_pool<BIST::ITCHOrderBookDirectoryMessage>().acquire(m);
    NASDAQ::ITCHStockDirectoryMessage *NQm = nullptr;
    engine->parser_dispatch_.itchparser_nasdaq_.itch_pools_.get_pool<NASDAQ::ITCHStockDirectoryMessage>().acquire(NQm);

    engine->parser_to_store_.push(MessageWithVenue<MessageTypes_t>{BIST::ITCHMessage{&test_data_ordMngr::bist_dir}, Venue::BIST});
    engine->ord_mngr_.store();
    engine->parser_to_store_.push(MessageWithVenue<MessageTypes_t>{NASDAQ::ITCHMessage{&test_data_ordMngr::nasdaq_dir}, Venue::NASDAQ});
    engine->ord_mngr_.store();

    fix_server->wait_accepting();
    ouch_bist_server->wait_accepting();
    ouch_nq_server->wait_accepting();

    engine->start();

    fix_server->wait_connecting();
    ouch_bist_server->wait_connecting();
    ouch_nq_server->wait_connecting();

    auto run_case = [&](int pkt_case)
    {
        uint64_t before = engine->risk_.pipeline_seq.load(std::memory_order_acquire);

        engine->ord_mngr_.add_awaitingAck_order(*test_data_builder::fix_new_order);
        engine->ord_mngr_.add_awaitingAck_order(*test_data_builder::ouch_new_order);
        engine->ord_mngr_.add_awaitingAck_order(*test_data_builder::NQouch_new_order);

        switch (pkt_case)
        {
            case 1: fix_server->send_to_client(test_data_network::fix_order_ack2);          break;
            case 2: ouch_bist_server->send_to_client(test_data_network::ouch_bist_order_ack); break;
            case 3: ouch_nq_server->send_to_client(test_data_network::ouch_nq_order_ack);    break;
            case 4: itch_bist_sender.send_to(test_data_network::itch_bist_add_order2);       break;
            case 5: itch_nq_sender.send_to(test_data_network::itch_nq_add_order);            break;
            default: __builtin_unreachable();
        }

        while (engine->risk_.pipeline_seq.load(std::memory_order_acquire) <= before)
            _mm_pause();

        size_t used = engine->risk_.orderrisk_next_slot;
        for (size_t i = 0; i < used; i++)
            engine->risk_.orderrisk_pool_[i & (ORDER_POOL_CAPACITY - 1)].active.store(false, std::memory_order_seq_cst);
        engine->risk_.orderrisk_next_slot = 0;

        const uint64_t hash = engine->hashtables_.hash_exec_id("EXEC00002");
        const size_t idx = hash & (HashTables::EXEC_ID_TABLE_SIZE - 1);
        for (size_t i = 0; i < HashTables::MAX_PROBE; i++)
            *(volatile uint64_t*)(&engine->hashtables_.exec_id_hashes_[(idx + i) & (HashTables::EXEC_ID_TABLE_SIZE - 1)]) = 0;

        engine->parser_dispatch_.flush_DbQueue();
        engine->session_manager_.getSessionState(0)->fix.set_expected_seq(1);
    };

    constexpr int WARMUP = 10'000;
    for (int i = 0; i < WARMUP; i++)
        run_case((i % 5) + 1);

    constexpr int N = 5'000'000;
    for (int i = 0; i < N; i++)
        run_case((i % 5) + 1);

    engine->stop();
    engine.reset();

    fix_server->stop_server();
    ouch_bist_server->stop_server();
    ouch_nq_server->stop_server();

    Logger::Shutdown();
    return 0;
}