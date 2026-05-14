#include "SessionManager.h"
#include "SoupBinTcp.h"
#include "Builder_OUCH_NASDAQ.h"
#include "dataset_builder.h"
#include "perf_utils.h"

#include <memory>
#include <vector>

int main()
{
    pin_to_cpu(2);

    auto sess_mngr          = std::make_unique<SessionManager>();
    auto sbt                = std::make_unique<SoupBinTcp>(*sess_mngr);
    auto builder_ouch_nasdaq = std::make_unique<Builder_OUCH_NASDAQ>(*sbt);

    std::vector<Order*> orders = {
        test_data_builder::NQouch_new_order,
        test_data_builder::NQouch_replace_order,
        test_data_builder::NQouch_modify_order,
        test_data_builder::NQouch_new_order,
        test_data_builder::NQouch_new_order,
        test_data_builder::NQouch_cancel_order,
        test_data_builder::NQouch_new_order,
        test_data_builder::NQouch_new_order
    };

    auto run = [&]()
    {
        asm volatile("" ::: "memory");
        for (auto* ord : orders)
            builder_ouch_nasdaq->build(*ord);
    };

    constexpr int WARMUP = 10'000;
    for (int i = 0; i < WARMUP; i++)
        run();

    constexpr int N = 5'000'000;
    for (int i = 0; i < N; i++)
        run();

    builder_ouch_nasdaq.reset();
    sbt.reset();
    sess_mngr.reset();

    return 0;
}