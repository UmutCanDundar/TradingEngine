
#include "Parser_FIX.h"
#include "dataset.h"
#include "bench_utils.h"


int main()
{
    pin_to_cpu(0);

    spscFIXInSessionQueue_t queue;
    Parser_FIX parser{queue};

    const auto* data = reinterpret_cast<const char*>(test_data::single_fix_pkt1.data());
    const auto  size = test_data::single_fix_pkt1.size();

    for (int i = 0; i < 5000000; i++)
    {
        auto* msg = parser.parse<FIXMessage>(data, size);
         volatile auto* sink = msg; 
        (void)sink;
        parser.releaseFIX(msg);
    }
}