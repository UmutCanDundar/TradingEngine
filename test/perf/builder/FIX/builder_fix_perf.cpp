#include "SessionManager.h"
#include "Builder_FIX.h"
#include "Order.h"
#include "dataset_builder.h"
#include "perf_utils.h"

#include <memory>

int main()
{
    pin_to_cpu(2);

    auto sess_mngr   = std::make_unique<SessionManager>();
    auto builder_fix = std::make_unique<Builder_FIX>(*sess_mngr);

    uint8_t sess_index = sess_mngr->getSessionIndex(Venue::BIST, Protocol::FIX);
    auto& seq_fix      = sess_mngr->getSessionState(sess_index)->fix;

    Order* order = test_data_builder::fix_new_order;

    auto run = [&]()
    {
        seq_fix.set_next_seq(1);
        asm volatile("" ::: "memory");
        builder_fix->build<FIXTypes::NewOrderSingle>(sess_index, *order);
    };

    constexpr int WARMUP = 10'000;
    for (int i = 0; i < WARMUP; i++)
        run();
       
    constexpr int N = 5'000'000;
    for (int i = 0; i < N; i++)
        run();

    builder_fix.reset();
    sess_mngr.reset();

    return 0;
}