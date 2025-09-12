#pragma once

void lock_memory() noexcept;

void configure_realtime(const int priority) noexcept;

void configure_affinity(const int cpu_id) noexcept;
