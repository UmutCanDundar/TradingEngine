#include "Parser_Dispatch.h"
#include "SessionManager.h"
#include "LoginController.h"
#include "SoupBinTcp.h"
#include "Builder_FIX.h"
#include "NetworkIO.h"
#include "Parser_FIX.h"
#include "dataset_parser.h"
#include "perf_utils.h"

#include <thread>
#include <atomic>
#include <array>
#include <vector>
#include <memory>

int main()
{
    pin_to_cpu(2);

    std::unique_ptr<TxPacketPoolManager> txPkt_pool;
    std::unique_ptr<SessionManager>      sess_mngr;
    std::unique_ptr<SoupBinTcp>          sbt;
    std::unique_ptr<Builder_FIX>         builder_fix;
    std::unique_ptr<LoginController>     login;
    std::unique_ptr<NetworkIO>           network_io;
    std::unique_ptr<Parser_FIX>          parser_fix;
    std::unique_ptr<Parser_Dispatch>     parser_dispatch;

    spscFIXInSessionQueue_t     parser_to_fixbuilder_in;
    spscRxPacketQueue_t        receiver_to_parser;
    spscTxPacketQueue_t         builder_to_sender;
    spscMessageQueue_t          parser_to_store;
    spscFIXOutSessionQueue_t    parser_to_fixbuilder_out;
    spscDbQueue_t               db_to_parser;

    std::atomic<bool> stop{false};
    std::thread consumer;

    std::atomic<bool> running{true};

    txPkt_pool  = std::make_unique<TxPacketPoolManager>();
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
                        *txPkt_pool,
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
    SessionState* sess_state = sess_mngr->getSessionState(sess_index);

    using PktVec = std::vector<RxPacket*>;
    std::array<PktVec, 4> cases = {{
        { &test_data_parser::fix_RxPacket_single_1 },
        { &test_data_parser::fix_RxPacket_full_1 },
        { &test_data_parser::fix_RxPacket_partial_1, &test_data_parser::fix_RxPacket_partial_2, &test_data_parser::fix_RxPacket_partial_3,
          &test_data_parser::fix_RxPacket_partial_4, &test_data_parser::fix_RxPacket_partial_5, &test_data_parser::fix_RxPacket_partial_6 }
    }};

    consumer = std::thread([&]
    {
        pin_to_cpu(0);
        MessageWithVenue<MessageTypes_t> local_msg;
        FIXSessionMessage* sesMsg;
        while (!stop.load(std::memory_order_acquire))
        {
            if (parser_to_store.pop(local_msg))
                parser_dispatch->fixparser_.releaseFIX(std::get<FIXMessage*>(local_msg.msg));
            else if (parser_to_fixbuilder_in.pop(sesMsg))
                parser_dispatch->fixparser_.releaseFIX(sesMsg);
            else
                _mm_pause();
        }
    });

    auto run_case = [&](int c)
    {
        auto& pkts = cases[c];
        for (auto* p : pkts) p->consumed = false;
        sess_state->fix.set_expected_seq(1);
        asm volatile("" ::: "memory");
        for (auto* p : pkts)
            parser_dispatch->parseFIX(p);
        while (!parser_to_store.empty() || !parser_to_fixbuilder_in.empty())
            _mm_pause();
    };

    constexpr int WARMUP = 10'000;
    for (int i = 0; i < WARMUP; i++)
        run_case(i % 3);

    constexpr int N = 5'000'000;
    for (int i = 0; i < N; i++)
        run_case(i % 3);

    stop.store(true, std::memory_order_release);
    consumer.join();

    parser_dispatch.reset();
    network_io.reset();
    login.reset();
    builder_fix.reset();
    sbt.reset();
    sess_mngr.reset();
    txPkt_pool.reset();

    return 0;
}