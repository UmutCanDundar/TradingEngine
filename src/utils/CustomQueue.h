// =============================================================================
// CustomQueue
//
// PURPOSE:
// - Lightweight, fixed-capacity ring buffer for pending elements.
// - Designed for low-latency, allocation-free temporary storage.
//
// THREAD SAFETY:
// - Not thread-safe.
// - Intended for single-threaded or externally synchronized usage.
//
// DEVELOPER NOTES:
// - Capacity N must be a power of two for mask-based indexing.
// - Intended specifically for NetworkIO.
//   Be careful before using in any other component.
// =============================================================================

#pragma once

#include <array>
#include <cstddef>
#include <type_traits>
#include <bit>

template <typename T, size_t N> requires (std::is_pointer_v<T> && std::has_single_bit(N)) 
class CustomQueue
{
    T buf[N];
    size_t head = 0;
    size_t tail = 0;

public:
    bool push(T b) noexcept
    {
        size_t next = (tail + 1) & (N - 1);
        if (next == head)
            return false; 
        buf[tail] = b;
        tail = next;
        return true;
    }

    bool pop(T &b) noexcept
    {
        if (head == tail)
            return false;
        b = buf[head];
        head = (head + 1) & (N - 1);
        return true;
    }

    void clear() noexcept
    {
        head = tail = 0;
    }

    bool empty() const noexcept
    {
        return head == tail;
    }

    size_t size() const noexcept
    {
        return (tail - head + N) & (N - 1);
    }

    T back() const noexcept
    {
        if (head == tail)
            return nullptr; 

        size_t back = (head == 0) ? head : (tail - 1);
        return buf[back];
    }

    T front() const noexcept
    {
        if (head == tail)
            return nullptr; 

        return buf[head];
    }

    void swap_last_two() noexcept
    {
        if ((tail == head) || ((tail + N - 1) & (N - 1)) == head)
            return;

        size_t pre_last = (tail + N - 2) & (N - 1);
        size_t last = (tail + N - 1) & (N - 1);
        std::swap(buf[pre_last], buf[last]);
    }
};

