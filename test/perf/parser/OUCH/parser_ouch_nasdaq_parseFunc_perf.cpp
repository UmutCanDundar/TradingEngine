#include "Parser_OUCH_NASDAQ.h"
#include "dataset_parser.h"
#include "perf_utils.h"

int main()
{
    pin_to_cpu(6);

    Parser_OUCH_NASDAQ parser{};

    std::vector<const uint8_t*> pkts = {
        test_data_parser::ouch_nasdaq_pkt1.data(),
        test_data_parser::ouch_nasdaq_pkt2.data(),
        test_data_parser::ouch_nasdaq_pkt3.data()
    };

    auto run = [&]()
    {
        for (auto* data : pkts)
        {
            auto msg = parser.parse(reinterpret_cast<const char*>(data));
            parser.releaseOUCH(msg);
        }
    };

    constexpr int WARMUP = 10'000;
    for (int i = 0; i < WARMUP; i++)
        run();

    constexpr int N = 5'000'000;
    for (int i = 0; i < N; i++)
        run();

    return 0;
}