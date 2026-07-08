#pragma once

#include <atomic>
#include <cstdint>
#include <type_traits>


template<typename T>
concept HashableKey =
    std::is_trivially_copyable_v<T> &&
    std::equality_comparable<T> &&
    sizeof(T) <= sizeof(uint64_t);

template<typename T>
concept TrivialValue = std::is_trivially_copyable_v<T>;

enum class SlotState : uint8_t 
{
    Empty = 0,  
    Valid = 1, 
};

template<HashableKey K, TrivialValue V>
struct alignas(64) Slot 
{
    static_assert(
        sizeof(K) + sizeof(V) + sizeof(std::atomic<SlotState>) <= 64,
        "Slot exceeds 64 byte CL — false sharing risk.\nUse smaller K/V or upgrade to 128-byte alignment."
    );
    
    K key_{};
    V value_{};
    std::atomic<SlotState> state_{SlotState::Empty};

    void insert_slot(const K& k, const V& v) noexcept 
    {
        key_ = k;
        value_ = v;
        state_.store(SlotState::Valid, std::memory_order_release);
    }

    void delete_slot() noexcept 
    {
        state_.store(SlotState::Empty, std::memory_order_release);
    }

    [[nodiscard]] bool is_valid() const noexcept 
    {
        return state_.load(std::memory_order_acquire) == SlotState::Valid;
    }

};


