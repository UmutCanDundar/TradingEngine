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
#include <vector>
#include <memory>

int main()
{
    pin_to_cpu(6);

    std::unique_ptr<TxPacketPoolManager> txPkt_pool;
    std::unique_ptr<SessionManager>      sess_mngr;
    std::unique_ptr<SoupBinTcp>          sbt;
    std::unique_ptr<Builder_FIX>         builder_fix;
    std::unique_ptr<LoginController>     login;
    std::unique_ptr<NetworkIO>           network_io;
    std::unique_ptr<Parser_FIX>          parser_fix;
    std::unique_ptr<Parser_Dispatch>     parser_dispatch;

    spscFIXTxSessionQueue_t     parser_to_fixbuilder_tx;
    spscRxPacketQueue_t        receiver_to_parser;
    spscTxPacketQueue_t         builder_to_sender;
    spscMessageQueue_t          parser_to_store;
    spscFIXRxSessionQueue_t    parser_to_fixbuilder_rx;
    spscDbQueue_t               db_to_parser;

    std::atomic<bool> stop{false};
    std::thread consumer;

    std::atomic<bool> running{true};

    txPkt_pool  = std::make_unique<TxPacketPoolManager>();
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

    std::vector<RxPacket*> pkts = {
        &test_data_parser::itch_nasdaq_RxPacket_single_1,
        &test_data_parser::itch_nasdaq_RxPacket_full_1
    };

    consumer = std::thread([&]
    {
        pin_to_cpu(0);
        MessageWithVenue<MessageTypes_t> local_msg;
        while (!stop.load(std::memory_order_acquire))
        {
            if (!parser_to_store.empty())
            {
                parser_to_store.pop(local_msg);
                parser_dispatch->itchparser_nasdaq_.releaseITCH(std::get<NASDAQ::ITCHMessage>(local_msg.msg));
            }
            else
                _mm_pause();
        }
    });
    
    auto run = [&]()
    {
        for (auto* p : pkts) p->msg_count = 0;
        asm volatile("" ::: "memory");
        for (auto* p : pkts)
            parser_dispatch->parseITCH_NASDAQ(p);
        while (!parser_to_store.empty()) _mm_pause();
    };

    constexpr int WARMUP = 10'000;
    for (int i = 0; i < WARMUP; i++)
        run();

    constexpr int N = 5'000'000;
    for (int i = 0; i < N; i++)
        run();

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