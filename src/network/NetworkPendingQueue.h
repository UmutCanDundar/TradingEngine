// =============================================================================
// NetworkPendingQueue
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

template <typename T, size_t N>
class PendingQueue
{
    T buf[N];
    size_t head = 0;
    size_t tail = 0;

public:
    inline bool push(T b) noexcept
    {
        size_t next = (tail + 1) & (N - 1);
        if (next == head)
            return false; // full
        buf[tail] = b;
        tail = next;
        return true;
    }

    inline bool pop(T &b) noexcept
    {
        if (head == tail)
            return false; // empty
        b = buf[head];
        head = (head + 1) & (N - 1);
        return true;
    }

    inline void clear() noexcept
    {
        head = tail = 0;
    }

    inline bool empty() noexcept
    {
        return head == tail;
    }

    inline T *back() noexcept
    {
        if (head == tail)
            return nullptr; // empty

        size_t back = (tail == 0) ? (N - 1) : (tail - 1);
        return &buf[back];
    }

    inline void swap_last_two() noexcept
    {
        if ((tail == head) || ((tail + N - 1) & (N - 1)) == head)
            return;

        size_t pre_last = (tail + N - 2) & (N - 1);
        size_t last = (tail + N - 1) & (N - 1);
        std::swap(buf[pre_last], buf[last]);
    }
};

