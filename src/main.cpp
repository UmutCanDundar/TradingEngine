#include "config_utils.h"
#include "MarketDataHandler.h"

#include <iostream>
#include <climits>

#include "deneme.h"

int main()
{
    configure_realtime(sched_get_priority_min(SCHED_FIFO));
    configure_affinity(7);

    // MarketDataHandler marketDataHandler;
    // marketDataHandler.run();

    x += 6;
    std::cout << x << std::endl;
    print();
    return 0;
}
