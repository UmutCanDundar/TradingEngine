#include <sched.h>
#include <emmintrin.h>

static void pin_to_cpu(int cpu_id, int priority = 99)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);
    sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);

    struct sched_param param{};
    param.sched_priority = priority;
    sched_setscheduler(0, SCHED_FIFO, &param);
    
   
}
