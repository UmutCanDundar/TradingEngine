#pragma once

#include "../common.h"

#include <cstddef>
#include <cstdint>
#include <cassert>
#include <memory>
#include <sys/mman.h>


// CPU friendly allocation: power-of-two, aligned.
template<size_t N>
struct is_power_of_two : std::bool_constant<(N > 0) && !(N & (N - 1))> {};

template<size_t N>
inline constexpr bool is_power_of_two_v = is_power_of_two<N>::value;

constexpr size_t align_up(size_t val, size_t align) noexcept 
{ 
    return (val + align - 1) & ~(align - 1); 
}

template<
    typename T,
    size_t Capacity,         
    size_t Alignment = alignof(T)    
>
class ArenaAllocator 
{

private:
    std::byte* buffer_ = nullptr;  
    std::byte* cursor_ = nullptr; 
    std::byte* end_    = nullptr;  

public:
    static_assert(Capacity > 0, "ArenaAllocator: capacity cannot be zero");
    static_assert(is_power_of_two_v<Capacity>, "ArenaAllocator: capacity must be a power of two");
    static_assert(is_power_of_two_v<Alignment>, "ArenaAllocator: alignment must be a power of two");
    static_assert(std::is_trivially_destructible_v<T>, "T must be trivially destructible");

    using value_type = T;
    using pointer    = T*;
    using size_type  = size_t;

    static constexpr size_t element_size = sizeof(T);
    static constexpr size_t aligned_size = align_up(element_size, Alignment);
    static constexpr size_t total_bytes  = aligned_size * Capacity;
    static constexpr size_t mmap_bytes   = align_up(total_bytes, 4096);

    ArenaAllocator() 
    {
        void* ptr = ::mmap(
            nullptr,
            mmap_bytes,
            PROT_READ | PROT_WRITE,
            MAP_ANONYMOUS | MAP_PRIVATE | MAP_POPULATE,
            -1, 0
        );
        assert(ptr != MAP_FAILED && "ArenaAllocator: mmap failed");

        buffer_ = static_cast<std::byte*>(ptr);
        cursor_ = buffer_;
        end_    = buffer_ + total_bytes;
    }

    ArenaAllocator(ArenaAllocator&& other) noexcept : buffer_(other.buffer_), cursor_(other.cursor_), end_(other.end_)
    {
        other.buffer_ = nullptr;
        other.cursor_ = nullptr;
        other.end_    = nullptr;
    }
    
    ArenaAllocator(const ArenaAllocator&) = delete;
    ArenaAllocator& operator=(const ArenaAllocator&) = delete;

    ~ArenaAllocator() {
        if (buffer_) {
            ::munmap(buffer_, mmap_bytes);
        }
    }

    [[nodiscard]] pointer allocate(size_type n = 1) noexcept {
        std::byte* result = cursor_;
        std::byte* next   = cursor_ + (aligned_size * n);

        if (UNLIKELY(next > end_))
            return nullptr;

        cursor_ = next;
        return reinterpret_cast<pointer>(result);
    }

    // Free once for all, there is no deallocation of individual elements.
    void deallocate([[maybe_unused]] pointer p, [[maybe_unused]] size_type n = 1) noexcept {} // std::allocator_traits compatibility but no-op
    void reset() noexcept { cursor_ = buffer_; }

    
    [[nodiscard]] size_type used_bytes() const noexcept { return cursor_ - buffer_; }
    [[nodiscard]] size_type used_count() const noexcept { return used_bytes() / aligned_size; }
    [[nodiscard]] size_type remaining_bytes() const noexcept { return end_ - cursor_; }
    [[nodiscard]] size_type capacity() const noexcept { return Capacity; }
};


