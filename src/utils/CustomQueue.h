// =============================================================================
// CustomQueue
//
// PURPOSE:
// - Lightweight, fixed-capacity ring buffer_fer for pending elements.
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
    T buffer_[N];
    size_t head_ = 0;
    size_t tail_ = 0;

public:
    bool push(T b) noexcept
    {
        size_t next = (tail_ + 1) & (N - 1);
        if (next == head_)
            return false; 
        buffer_[tail_] = b;
        tail_ = next;
        return true;
    }

    bool pop(T &b) noexcept
    {
        if (head_ == tail_)
            return false;
        b = buffer_[head_];
        head_ = (head_ + 1) & (N - 1);
        return true;
    }

    void clear() noexcept
    {
        head_ = tail_ = 0;
    }

    bool empty() const noexcept
    {
        return head_ == tail_;
    }

    size_t size() const noexcept
    {
        return (tail_ - head_ + N) & (N - 1);
    }

    T back() const noexcept
    {
        if (head_ == tail_)
            return nullptr; 

        size_t back = (head_ == 0) ? head_ : (tail_ - 1);
        return buffer_[back];
    }

    T front() const noexcept
    {
        if (head_ == tail_)
            return nullptr; 

        return buffer_[head_];
    }

    void swap_last_two() noexcept
    {
        if ((tail_ == head_) || ((tail_ + N - 1) & (N - 1)) == head_)
            return;

        size_t pre_last = (tail_ + N - 2) & (N - 1);
        size_t last = (tail_ + N - 1) & (N - 1);
        std::swap(buffer_[pre_last], buffer_[last]);
    }
};

