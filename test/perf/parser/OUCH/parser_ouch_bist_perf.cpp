#include "Parser_Dispatch.h"
#include "SessionManager.h"
#include "LoginController.h"
#include "SoupBinTcp.h"
#include "Builder_FIX.h"
#include "NetworkIO.h"
#include "Parser_FIX.h"
#include "Logger.h"
#include "dataset_parser.h"
#include "perf_utils.h"

#include <thread>
#include <atomic>
#include <vector>
#include <memory>

int main()
{
    pin_to_cpu(2);

    std::unique_ptr<InPacketPoolManager> inPkt_pool;
    std::unique_ptr<SessionManager>      sess_mngr;
    std::unique_ptr<SoupBinTcp>          sbt;
    std::unique_ptr<Builder_FIX>         builder_fix;
    std::unique_ptr<LoginController>     login;
    std::unique_ptr<NetworkIO>           network_io;
    std::unique_ptr<Parser_FIX>          parser_fix;
    std::unique_ptr<Parser_Dispatch>     parser_dispatch;

    spscFIXInSessionQueue_t     parser_to_fixbuilder_in;
    spscOutPacketQueue_t        receiver_to_parser;
    spscInPacketQueue_t         builder_to_sender;
    spscMessageQueue_t          parser_to_store;
    spscFIXOutSessionQueue_t    parser_to_fixbuilder_out;
    spscDbQueue_t               db_to_parser;

    std::atomic<bool> stop{false};
    std::atomic<bool> stop2{false};
    std::atomic<bool> trigger_network{false};
    std::atomic<bool> running{true};

    std::thread consumer;
    std::thread network_thread;

    PendingQueue<OutPacket*, 256> pend_read;
    uint8_t sess_index;
    ssize_t rest_ouch_msg = 0;

    inPkt_pool  = std::make_unique<InPacketPoolManager>();
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

    sess_index = sess_mngr->getSessionIndex(Venue::BIST, Protocol::OUCH);

    std::vector<OutPacket*> pkts = {
        &test_data_parser::ouch_bist_outpacket_single_1,
        &test_data_parser::ouch_bist_outpacket_full_1,
        &test_data_parser::ouch_bist_outpacket_partial_1,
        &test_data_parser::ouch_bist_outpacket_partial_2,
        &test_data_parser::ouch_bist_outpacket_partial_3
    };

    network_thread = std::thread([&]
    {
        pin_to_cpu(4);
        bool sbt_ret = true;
        while (!stop2.load(std::memory_order_acquire))
        {
            if (trigger_network.load(std::memory_order_acquire))
            {
                for (auto* pkt : pkts)
                {
                    sbt_ret = network_io->SBTPacketHandler(pkt, pkt->len, pend_read, sess_index);
                    if (sbt_ret)
                        receiver_to_parser.push(pkt);
                }
                trigger_network.store(false, std::memory_order_release);
            }
            else
                _mm_pause();
        }
    });

    consumer = std::thread([&]
    {
        pin_to_cpu(0);
        MessageWithVenue<MessageTypes_t> local_msg;
        while (!stop.load(std::memory_order_acquire))
        {
            if (!parser_to_store.empty())
            {
                parser_to_store.pop(local_msg);
                parser_dispatch->ouchparser_bist_.releaseOUCH(std::get<BIST::OUCHMessage>(local_msg.msg));
            }
            else
                _mm_pause();
        }
    });

    auto run = [&]()
    {
        while (trigger_network.load(std::memory_order_acquire)) _mm_pause();
        std::atomic_thread_fence(std::memory_order_seq_cst);

        for (auto* p : pkts) { p->msg_count = 0; p->consumed = false; }
        rest_ouch_msg = static_cast<ssize_t>(pkts.size());

        asm volatile("" ::: "memory");
        trigger_network.store(true, std::memory_order_release);

        while (rest_ouch_msg > 0)
        {
            if (parser_dispatch->dispatch()) rest_ouch_msg--;
            else _mm_pause();
        }
        while (!parser_to_store.empty()) _mm_pause();
    };

    constexpr int WARMUP = 10'000;
    for (int i = 0; i < WARMUP; i++)
        run();

    constexpr int N = 5'000'000;
    for (int i = 0; i < N; i++)
        run();

    stop.store(true, std::memory_order_release);
    stop2.store(true, std::memory_order_release);
    consumer.join();
    network_thread.join();

    parser_dispatch.reset();
    network_io.reset();
    login.reset();
    builder_fix.reset();
    sbt.reset();
    sess_mngr.reset();
    inPkt_pool.reset();

    return 0;
}