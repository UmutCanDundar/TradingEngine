// =========================================================================================================
// config_utils
//
// PURPOSE:
// - Provides low-level OS configuration helpers for latency-sensitive environments.
// - Includes memory locking, real-time scheduling, and CPU affinity configuration.
//
// DESIGN NOTES:
// - Stateless utility functions.
// - Intended to be called once during engine initialization.
// - Linux-specific (mlockall, SCHED_FIFO, pthread_setaffinity_np).
//
// =========================================================================================================

#pragma once

void lock_memory();

void configure_realtime(int priority);

void configure_affinity(int cpu_id);
