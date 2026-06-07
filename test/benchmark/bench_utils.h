#include <x86intrin.h>
#include <sched.h>
#include <emmintrin.h>
#include <algorithm>
#include <numeric>
// #include <vector>

static void pin_to_cpu(int cpu_id, int priority = 99) 
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);
    sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);
    
    struct sched_param param{};
    param.sched_priority = priority;
    sched_setscheduler(0, SCHED_RR, &param);
}

// ==========================
// RDTSC
// ==========================
static inline uint64_t rdtsc_start()
{
    unsigned aux;
    return __rdtscp(&aux);
}

static inline uint64_t rdtsc_end()
{
    unsigned aux;
    uint64_t t = __rdtscp(&aux);
    asm volatile("mfence" ::: "memory");
    return t;
}

// // ==========================
// // CACHE FLUSH (approx)
// // ==========================
// static void flush_cache()
// {
//    static std::vector<char> buffer(32 * 1024 * 1024);

//     for (size_t i = 0; i < buffer.size(); i += 64)
//         _mm_clflush(&buffer[i]);
// }

// // ==========================
// // CORE BENCH FUNCTION
// // ==========================
// enum class CacheState
// {
//     Warm = 0,
//     Cold = 1
// };