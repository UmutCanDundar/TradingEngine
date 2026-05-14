#include "SessionManager.h"
#include "SoupBinTcp.h"
#include "Builder_OUCH_BIST.h"
#include "dataset_builder.h"
#include "perf_utils.h"

#include <memory>
#include <vector>

int main()
{
    pin_to_cpu(2);

    auto sess_mngr         = std::make_unique<SessionManager>();
    auto sbt               = std::make_unique<SoupBinTcp>(*sess_mngr);
    auto builder_ouch_bist = std::make_unique<Builder_OUCH_BIST>(*sbt, *sess_mngr);

    
    std::vector<Order*> orders = {
        test_data_builder::ouch_new_order,
        test_data_builder::ouch_replace_order,
        test_data_builder::ouch_cancelid_order,
        test_data_builder::ouch_new_order,
        test_data_builder::ouch_new_order,
        test_data_builder::ouch_cancel_order,
        test_data_builder::ouch_new_order,
        test_data_builder::ouch_new_order
    };

    auto run = [&]()
    {
        asm volatile("" ::: "memory");
        for (auto* ord : orders)
            builder_ouch_bist->build(*ord);
    };

    constexpr int WARMUP = 10'000;
    for (int i = 0; i < WARMUP; i++)
        run();

    constexpr int N = 5'000'000;
    for (int i = 0; i < N; i++)
        run();

    builder_ouch_bist.reset();
    sbt.reset();
    sess_mngr.reset();

    return 0;
}