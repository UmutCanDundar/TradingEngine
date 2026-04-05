// =============================================================================
// MessagePools / PoolAndFreeList
//
// PURPOSE:
// - Provides fixed-capacity object pools for protocol-specific message types.
// - Avoids dynamic allocations in hot paths by reusing preallocated objects.
// - Groups multiple message-type pools using a tuple-based compile-time layout.
//
// THREAD SAFETY:
// - Each PoolAndFreeList is intended for single-producer / single-consumer usage.
// - Thread safety is guaranteed by boost::lockfree::spsc_queue semantics.
//
// PERFORMANCE CHARACTERISTICS:
// - Allocation-free acquire/release after initialization.
// - Pointer-based handoff (T*) avoids copies and object moves.
// - Designed for low-latency message passing between pipeline stages.
//
// DEVELOPER NOTES (PRE-PROFILING):
// - POOL_CAPACITY is currently fixed at 512 for all message types.
//   This may be over-provisioned for some message kinds and should be revisited
//   after observing real traffic patterns (e.g., ITCH Add/Exec/Cancel ratios).
// =============================================================================

#pragma once

#include <tuple>
#include <array>

#include <boost/lockfree/spsc_queue.hpp>

template <typename T>
struct PoolAndFreeList
{
    static constexpr size_t POOL_CAPACITY = 512;

    std::array<T, POOL_CAPACITY> pool_;
    boost::lockfree::spsc_queue<T *> free_list_{POOL_CAPACITY};

    void initialize()
    {
        for (size_t i = 0; i < POOL_CAPACITY; ++i)
            free_list_.push(&pool_[i]);
    }

    void acquire(T*& obj) noexcept
    {
        free_list_.pop(obj);
    }

    void release(T*& obj) noexcept
    {
        free_list_.push(obj);
    }
};

template <typename T>
struct MessagePools;

template <typename... MsgTypes>
struct MessagePools<std::tuple<MsgTypes...>>
{
    std::tuple<PoolAndFreeList<MsgTypes>...> pool_and_freelist_;

    void initialize_all()
    {
        (std::get<PoolAndFreeList<MsgTypes>>(pool_and_freelist_).initialize(), ...);
    }

    template <typename T>
    PoolAndFreeList<T>& get_pool() noexcept
    {
        return std::get<PoolAndFreeList<T>>(pool_and_freelist_);
    }
};
