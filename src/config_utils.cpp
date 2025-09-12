#include "config_utils.h"
#include "ErrorHandler.h"

#include <cerrno>
#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>
#include <unistd.h>

void lock_memory() noexcept
{
   if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0)
   {
      ErrorHandler::handleError(errno);
   }
}

void configure_realtime(const int priority) noexcept
{
   struct sched_param sched;
   sched.sched_priority = priority;

   if (sched_setscheduler(0, SCHED_FIFO, &sched) != 0)
   {
      ErrorHandler::handleError(errno);
   }
}

void configure_affinity(const int cpu_id) noexcept
{
   cpu_set_t cpuset;
   CPU_ZERO(&cpuset);
   CPU_SET(cpu_id, &cpuset);

   pthread_t thread = pthread_self();
   if (pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset) != 0)
   {
      ErrorHandler::handleError(errno);
   }
}