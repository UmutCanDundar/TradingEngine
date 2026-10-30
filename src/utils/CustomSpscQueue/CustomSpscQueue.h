// ================================================================================
// CustomSpscQueue<T, N>
//
// Wait-free single-producer / single-consumer ring buffer for pointer payloads.
//
// Design:
//   - The slot itself is the synchronization point: nullptr = empty, non-null = full.
//     Producer and consumer never read each other's index, so write_idx_ and
//     read_idx_ are plain size_t (each has exactly one writer thread).
//   - Slot publication uses release/acquire; the pointee's contents are made
//     visible to the consumer by the same release store.
//   - write_idx_, read_idx_ and buffer_ each live on their own 64-byte cache line
//     (no false sharing between the two threads' indices).
//   - Consecutive logical indices are remapped to different cache lines
//     (3-bit rotation), so a near-empty queue does not ping-pong a single line
//     between producer and consumer. Memory footprint stays 8 B/slot.
//
// Thread ownership:
//   - Producer only: push, full
//   - Consumer only: pop, empty
//   - Nobody running: reset_unsafe (call before threads start / after join)
//   - Debug builds assert on wrong-thread calls; release builds have zero overhead.
//   - reset_unsafe also clears the debug thread-owner ids, so the queue can be
//     reused by different threads after a reset.
//
// Constraints:
//   - N must be a power of two, N >= 8, compile-time constant
//   - T must be a pointer type; nullptr cannot be pushed (it marks an empty slot)
//   - empty()/full() are approximate by nature; prefer checking push()/pop() results
//   - reset_unsafe() does not release queued pointers; drain with pop() first
// ================================================================================
#pragma once

#include <cstddef>
#include <concepts>
#include <type_traits>
#include <bit>
#include <atomic>
#include <thread>
#include <cassert>

template<typename T, std::size_t N>
requires (std::has_single_bit(N) && N >= 8 && std::is_pointer_v<T>)
class CustomSpscQueue
{
public:
    using value_type = T;
    using size_type  = std::size_t;

private:
    static constexpr size_type L_BITS = 3; // Representing the slot number in the cache-line
    static constexpr size_type H_BITS = std::countr_one(N - 1) - L_BITS; // Representing the number of the cache-line

    alignas(64) size_type read_idx_{0};
    alignas(64) size_type write_idx_{0};
    alignas(64) std::atomic<value_type> buffer_[N]{};

public:
    CustomSpscQueue() noexcept = default;
    CustomSpscQueue(const CustomSpscQueue&) = delete;
    CustomSpscQueue(CustomSpscQueue&&) = delete;

    bool push(value_type elem) noexcept // only producer
    {
        check_producer();
        assert(elem != nullptr);

        auto& slot = buffer_[remap(write_idx_)];

        if (slot.load(std::memory_order_acquire))
            return false;

        slot.store(elem, std::memory_order_release);
        ++write_idx_;

        return true;
    }

    bool pop(value_type& out) noexcept // only consumer
    {
        check_consumer();

        auto& slot = buffer_[remap(read_idx_)];
        value_type elem = slot.load(std::memory_order_acquire);

        if (!elem)
            return false;

        slot.store(nullptr, std::memory_order_release);
        out = elem;
        ++read_idx_;

        return true;
    }

    bool empty() const noexcept // only consumer
    {
        check_consumer();
        return buffer_[remap(read_idx_)].load(std::memory_order_relaxed) == nullptr;
    }

    bool full() const noexcept // only producer
    {
        check_producer();
        return buffer_[remap(write_idx_)].load(std::memory_order_relaxed) != nullptr;
    }

    void reset_unsafe() noexcept
    {
        for (auto& slot : buffer_)
            slot.store(nullptr, std::memory_order_relaxed);
        write_idx_ = 0;
        read_idx_ = 0;

        #ifndef NDEBUG
        producer_id_ = std::thread::id{};
        consumer_id_ = std::thread::id{};
        #endif
    }

private:
    static constexpr size_type remap(size_type i) noexcept
    {
        const size_type idx = i & (N - 1);
        return ((idx & 7) << H_BITS) | (idx >> L_BITS);
    }

#ifndef NDEBUG
    mutable std::thread::id producer_id_{}, consumer_id_{};

    void check_producer() const noexcept
    {
        auto id = std::this_thread::get_id();
        if (producer_id_ == std::thread::id{}) producer_id_ = id;
        assert(producer_id_ == id && "push/full called not from producer thread");
    }
    void check_consumer() const noexcept
    {
        auto id = std::this_thread::get_id();
        if (consumer_id_ == std::thread::id{}) consumer_id_ = id;
        assert(consumer_id_ == id && "pop/empty called not from consumer thread");
    }
#else
    void check_producer() const noexcept {}
    void check_consumer() const noexcept {}
#endif
};



