#include <gtest/gtest.h>

#include "OrderManager.h"
#include "MarketBook.h"
#include "HashTables.h"
#include "Order.h"
#include "dataset_network.h"
#include "dataset_builder.h"
#include "SessionManager.h"
#include "LoginController.h"
#include "SoupBinTcp.h"
#include "Builder_FIX.h"
#include "NetworkPackets.h"
#include "FIXMessage.h"
#include "Builder_Dispatch.h"
#include "Logger.h"
#include "NetworkIO.h"
#include "Parser_FIX.h"

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
    static constexpr uint16_t FAKE_FIX_PORT       = 9876;  // session 0 - FIX TCP
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
        std::mutex listen_mutex;
        std::condition_variable listening_cv;

        std::atomic<bool> accepted{false};
        std::mutex accept_mutex;
        std::condition_variable accepting_cv;

        std::atomic<bool> connected{false};
        std::mutex connect_mutex;
        std::condition_variable connecting_cv;

        std::thread accept_thread;
        std::thread recv_thread;
        std::thread send_thread;

        std::vector<std::vector<uint8_t>> received_data;
        std::mutex recv_mutex;
        std::condition_variable recv_cv;

        std::queue<std::vector<uint8_t>> send_queue;
        std::mutex send_mutex;
        std::condition_variable send_cv;

        std::mutex states_mutex;
        std::condition_variable states_ready_cv;

        std::mutex& packet_mutex;
        std::condition_variable& packets_ready_cv;


        FakeTCPServer(uint16_t port, std::mutex& mutex, std::condition_variable& cv) : port(port), packet_mutex(mutex), packets_ready_cv(cv) {}
    
        void wait_accepting() 
        {
            std::unique_lock<std::mutex> lk(accept_mutex);
            accepting_cv.wait(lk, [this]{
                return accepted.load(std::memory_order_acquire);
            });
        }

        bool wait_connecting() 
        {
            std::unique_lock<std::mutex> lk (connect_mutex);
            connecting_cv.wait(lk, [this](){ return connected.load(std::memory_order_acquire); });
                        
            return connected.load(std::memory_order_acquire);
        }
        
        bool wait_logging(SessionManager& sess_mngr, const uint8_t index) {
            std::unique_lock<std::mutex> lk(states_mutex);
            states_ready_cv.wait(lk, [&sess_mngr, index](){return sess_mngr.isSessionLoggedIn(index); });

            return sess_mngr.isSessionLoggedIn(index);                                                                              
        }

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

            accept_thread = std::thread([this]() {
       
                sockaddr_in client_addr{};
                socklen_t len = sizeof(client_addr);
                
                accepted.store(true, std::memory_order_release);
                accepting_cv.notify_all();

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
                connecting_cv.notify_all();
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


TEST(NetworkIOTest, NetworkIOTestAll)
{
    Logger::Init();

    auto txPkt_pool = std::make_unique<TxPacketPoolManager>();  
    //Order Manager queues
    spscOrderQueue_t store_to_strategy_free_slot;
    spscOrderQueue_t store_to_risk;
    spscOrderQueue_t store_to_strategy;
    spscMessageQueue_t parser_to_store;
    spscDbQueue_t store_to_db;

    //Builder Dispatch queues
    spscFIXInSessionQueue_t parser_to_fixbuilder_in;
    spscFIXOutSessionQueue_t parser_to_fixbuilder_out;
    spscTxPacketQueue_t builder_to_sender;
    spscOrderQueue_t risk_to_builder;

    //NetworkIO queues
    // spscTxPacketQueue_t builder_to_sender;
    spscRxPacketQueue_t receiver_to_parser;    
    
    std::atomic<bool> running{true};
    
    auto hashtables    = std::make_unique<HashTables>();
    auto marketbook    = std::make_unique<MarketBook>(*hashtables);
    auto sess_mngr     = std::make_unique<SessionManager>();
    auto sbt           = std::make_unique<SoupBinTcp>(*sess_mngr);
    auto builder_fix   = std::make_unique<Builder_FIX>(*sess_mngr);
    auto login         = std::make_unique<LoginController>(*sbt, *builder_fix, *sess_mngr);
    auto parser_fix    = std::make_unique<Parser_FIX>(parser_to_fixbuilder_in);
    
    auto order_manager = std::make_unique<OrderManager>(
                                        parser_to_store,
                                        store_to_strategy,
                                        store_to_strategy_free_slot,
                                        store_to_risk,
                                        store_to_db,
                                        *marketbook,
                                        *hashtables        
    );

    auto builder_dispatch = std::make_unique<Builder_Dispatch>(
                                        builder_to_sender,
                                        risk_to_builder,
                                        parser_to_fixbuilder_out,
                                        parser_to_fixbuilder_in,
                                        *sess_mngr,
                                        *sbt,
                                        *login,
                                        *txPkt_pool,
                                        *builder_fix,
                                        *order_manager,
                                        *parser_fix        
    );

    std::mutex packet_mutex;
    std::condition_variable packets_ready_cv;

    FakeTCPServer fix_server(FAKE_FIX_PORT, packet_mutex, packets_ready_cv);
    FakeTCPServer ouch_bist_server(FAKE_OUCH_BIST_PORT, packet_mutex, packets_ready_cv);
    FakeTCPServer ouch_nq_server(FAKE_OUCH_NQ_PORT, packet_mutex, packets_ready_cv);
    FakeUDPSender itch_bist_sender(FAKE_ITCH_BIST_PORT);
    FakeUDPSender itch_nq_sender(FAKE_ITCH_NQ_PORT);

    fix_server.start();
    ouch_bist_server.start();
    ouch_nq_server.start();

    auto network_io = std::make_unique<NetworkIO>(
                                        receiver_to_parser,
                                        builder_to_sender,
                                        *sess_mngr,
                                        *sbt,
                                        *login,
                                        *txPkt_pool,
                                        running
    );

    EXPECT_TRUE(network_io->socket_states_[0].connection_pending);
    EXPECT_TRUE(network_io->socket_states_[1].connection_pending);
    EXPECT_TRUE(network_io->socket_states_[2].connection_pending);
    EXPECT_EQ(network_io->socket_states_[0].epoll_data, (0ULL | static_cast<uint64_t>(Venue::BIST) << 8 | static_cast<uint64_t>(Protocol::FIX) << 16));
    EXPECT_EQ(network_io->socket_states_[1].epoll_data, (1ULL | static_cast<uint64_t>(Venue::BIST) << 8 | static_cast<uint64_t>(Protocol::OUCH) << 16));
    EXPECT_EQ(network_io->socket_states_[2].epoll_data, (2ULL | static_cast<uint64_t>(Venue::NASDAQ) << 8 | static_cast<uint64_t>(Protocol::OUCH) << 16));
    // All networkIO sockets are created and ::connect the server sockets(EINPROGRESS)

    fix_server.wait_accepting();
    ouch_bist_server.wait_accepting();
    ouch_nq_server.wait_accepting();
    //Wait until all server sockets call ::accept  

    std::thread nio_thread([&]() {
        network_io->recv_send();
    });
    // NetworkIO starts (epoll starting point)(handling EINPROGRESS)

    EXPECT_TRUE(fix_server.wait_connecting());
    EXPECT_TRUE(ouch_bist_server.wait_connecting());
    EXPECT_TRUE(ouch_nq_server.wait_connecting());
    // Wait until all handshakes are completed
    
    {
        std::unique_lock<std::mutex> lk(fix_server.recv_mutex);
        fix_server.recv_cv.wait(lk, [&]{ 
            return fix_server.received_data.size() >= 1; 
        });
    }
    {
        std::unique_lock<std::mutex> lk(ouch_bist_server.recv_mutex);
        ouch_bist_server.recv_cv.wait(lk, [&]{ 
            return ouch_bist_server.received_data.size() >= 1; 
        });
    }
    {
        std::unique_lock<std::mutex> lk(ouch_nq_server.recv_mutex);
        ouch_nq_server.recv_cv.wait(lk, [&]{ 
            return ouch_nq_server.received_data.size() >= 1; 
        });
    }
    // Wait until all login messages arrived to the servers

    sess_mngr->setSessionLoggedIn(0, true); // FIX SessionMessages(Login Ack) is handled by Parser_FIX, so FIX SessionState is updated manually 
    ouch_bist_server.send_to_client(test_data_network::sbt_login_accepted_bist);
    ouch_nq_server.send_to_client(test_data_network::sbt_login_accepted_nq);
    // Imitating Login Ack from servers
   
    EXPECT_TRUE(fix_server.wait_logging(*sess_mngr, 0));
    EXPECT_TRUE(ouch_bist_server.wait_logging(*sess_mngr, 1));
    EXPECT_TRUE(ouch_nq_server.wait_logging(*sess_mngr, 2));
    // Wait until SBTPacketHandler handles login ack packets   

    // ====================================================
    //       v TRAFFIC FOR NETWORK IO SENDER v 
        std::array<Order*, 3> orders = {
            test_data_builder::fix_new_order, 
            test_data_builder::ouch_new_order,
            test_data_builder::NQouch_new_order
        };
        
        for (auto ord : orders) 
        {  
            risk_to_builder.push(ord);
            builder_dispatch->dispatch();
        }
    //       ^ TRAFFIC FOR NETWORK IO SENDER ^
    // ====================================================

    {
        std::unique_lock<std::mutex> lk(fix_server.recv_mutex);
        fix_server.recv_cv.wait(lk, [&fix_server](){return (fix_server.received_data.size() == 2); });   
    }
    {
        std::unique_lock<std::mutex> lk(ouch_bist_server.recv_mutex);
        ouch_bist_server.recv_cv.wait(lk, [&ouch_bist_server](){return (ouch_bist_server.received_data.size() == 2); });
    }
    {
        std::unique_lock<std::mutex> lk(ouch_nq_server.recv_mutex);
        ouch_nq_server.recv_cv.wait(lk, [&ouch_nq_server](){return (ouch_nq_server.received_data.size() == 2); });
    }
    // Wait until all order messages arrived to the servers
    

    // ====================================================
    //       v TRAFFIC FOR NETWORK IO RECEIVER v 
        std::array<const std::vector<uint8_t>*, 5> msgs = {
            &test_data_network::fix_order_ack, 
            &test_data_network::ouch_bist_order_ack,
            &test_data_network::ouch_nq_order_ack,
            &test_data_network::itch_bist_add_order,
            &test_data_network::itch_nq_add_order
        };

        fix_server.send_to_client(*msgs[0]);
        ouch_bist_server.send_to_client(*msgs[1]);
        ouch_nq_server.send_to_client(*msgs[2]);
        itch_bist_sender.send_to(*msgs[3]);
        itch_nq_sender.send_to(*msgs[4]);               
    //       ^ TRAFFIC FOR NETWORK IO RECEIVER ^
    // ====================================================,

    std::vector<RxPacket*> rxPkts;
    rxPkts.reserve(5);
    
    while (rxPkts.size() < 5) 
    {
        RxPacket* pkt = nullptr;
        while (receiver_to_parser.pop(pkt))
            rxPkts.push_back(pkt);
        
        if (rxPkts.size() >= 5) break;

        std::unique_lock<std::mutex> lk(packet_mutex);
        packets_ready_cv.wait(lk);  
    }

    auto check_fix_ack = [](const RxPacket* rxPkt)
    {
        EXPECT_EQ(rxPkt->len,                    125);
        EXPECT_EQ(rxPkt->data[19],               '8');       // 35=MsgType = ExecutionReport
        EXPECT_EQ(rxPkt->data[78],               'G');       // 55=Symbol = GARAN
        EXPECT_EQ(rxPkt->data[87],               '1');       // 54=Side = Buy
        EXPECT_EQ(rxPkt->data[92],               '1');       // 38=Qty starts with '1' (100)
        EXPECT_EQ(rxPkt->data[110],              '0');       // 39=OrdStatus = New
        EXPECT_EQ(rxPkt->venue,                  Venue::BIST);
        EXPECT_EQ(rxPkt->protocol,               Protocol::FIX);
    };
    auto check_ouch_bist_ack = [](const RxPacket* rxPkt)
    {
        EXPECT_EQ(rxPkt->len,                    140);
        EXPECT_EQ(rxPkt->data[3],                'A');       // message_type
        EXPECT_EQ(rxPkt->data[30],               'B');       // side = Buy
        EXPECT_EQ(rxPkt->data[46],               0x64);      // quantity last byte = 100
        EXPECT_EQ(rxPkt->data[49],               0x27);      // price high byte
        EXPECT_EQ(rxPkt->data[50],               0x10);      // price low byte = 10000
        EXPECT_EQ(rxPkt->venue,                  Venue::BIST);
        EXPECT_EQ(rxPkt->protocol,               Protocol::OUCH);
    };
    auto check_ouch_nq_ack = [](const RxPacket* rxPkt)
    {
        EXPECT_EQ(rxPkt->len,                    67);
        EXPECT_EQ(rxPkt->data[3],                'A');       // message_type
        EXPECT_EQ(rxPkt->data[16],               'S');       // side = Sell
        EXPECT_EQ(rxPkt->data[20],               0x64);      // quantity last byte = 100
        EXPECT_EQ(rxPkt->venue,                  Venue::NASDAQ);
        EXPECT_EQ(rxPkt->protocol,               Protocol::OUCH);
    };
    auto check_itch_bist_add = [](const RxPacket* rxPkt)
    {
        EXPECT_EQ(rxPkt->len,                    45);
        EXPECT_EQ(rxPkt->data[0],                'A');       // message_type
        EXPECT_EQ(rxPkt->data[17],               'B');       // side = Buy
        EXPECT_EQ(rxPkt->data[29],               0x64);      // quantity last byte = 100
        EXPECT_EQ(rxPkt->data[30],               0x00);      // price[0]
        EXPECT_EQ(rxPkt->data[33],               0x10);      // price last byte
        EXPECT_EQ(rxPkt->venue,                  Venue::BIST);
        EXPECT_EQ(rxPkt->protocol,               Protocol::ITCH);
    };
    auto check_itch_nq_add = [](const RxPacket* rxPkt)
    {
        EXPECT_EQ(rxPkt->len,                    36);
        EXPECT_EQ(rxPkt->data[0],                'A');       // message_type
        EXPECT_EQ(rxPkt->data[19],               'S');       // side = Sell
        EXPECT_EQ(rxPkt->data[23],               0x64);      // shares last byte = 100
        EXPECT_EQ(rxPkt->data[24],               'A');       // stock[0] = A
        EXPECT_EQ(rxPkt->data[32],               0x05);      // price[0]
        EXPECT_EQ(rxPkt->data[35],               0x00);      // price last byte
        EXPECT_EQ(rxPkt->venue,                  Venue::NASDAQ);
        EXPECT_EQ(rxPkt->protocol,               Protocol::ITCH);
    };
    using checkFunc = std::function<void(const RxPacket*)>;
    std::array<checkFunc, 5> checks
    {
        check_fix_ack,
        check_ouch_bist_ack,
        check_ouch_nq_ack,
        check_itch_bist_add,
        check_itch_nq_add
    };

    for(const auto* rxPkt : rxPkts) 
    {
        auto venue = rxPkt->venue;
        auto prot = rxPkt->protocol;

        if(venue == Venue::BIST)
        {
            switch(prot)
            {
                case Protocol::FIX: checks[0](rxPkt); continue;
                case Protocol::OUCH: checks[1](rxPkt); continue;
                case Protocol::ITCH: checks[3](rxPkt); continue;
                default: continue;
            }
        }
        else
        {
            switch(prot)
            {
                case Protocol::OUCH: checks[2](rxPkt); continue;
                case Protocol::ITCH: checks[4](rxPkt); continue;
                default: continue;
            }   
        }
    }
    

    fix_server.stop_server();
    ouch_bist_server.stop_server();
    ouch_nq_server.stop_server();
    
    running.store(false, std::memory_order_release);
    if(nio_thread.joinable()) nio_thread.join();

    Logger::Shutdown();  
}

