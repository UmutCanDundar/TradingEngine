// ================================================================================
// ConcurrentFixedFlatMap<K, V, N, Hash>
//
// Guarantees:
//   - Single writer thread, single reader thread (SPSC)
//   - Lock-free (atomic store/load, no mutex)
//   - No false sharing (64-byte aligned slots)
//   - Linear probing with load factor control
//   - O(1) amortized insert/lookup/erase
//
// Constraints:
//   - N must be power-of-two and compile-time constant
//   - K: trivially copyable, equality comparable, ≤8 bytes
//   - V: trivially copyable
//   - Single concurrent insert (writer), single concurrent lookup/erase (reader)
// ================================================================================

#pragma once

#include "slot.h"

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <utility>
#include <functional>
#include <type_traits>
#include <bit>

template<
    HashableKey K,
    TrivialValue V,
    size_t N = 64,
    typename Hash = std::hash<K>
>
class ConcurrentFixedFlatMap 
{

public:

    using key_type   = K;
    using value_type = V;
    using slot_type  = Slot<K, V>;
    using hash_type  = Hash;
    using size_type  = size_t;

    // ===================================================================================
    // Iterator for ConcurrentFixedFlatMap to compatible with range-based for loops and STL.
    // ===================================================================================
    struct iterator
    {
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type        = std::pair<K, V>;
        using difference_type   = std::ptrdiff_t;
        using reference         = value_type;

    private:
        slot_type* ptr_;
        slot_type* begin_;
        slot_type* end_;

        void advance_to_valid() noexcept
        {
            while (ptr_ != end_ && !ptr_->is_valid())
                ++ptr_;
        }

    public:
        struct arrow_proxy
        {
            value_type val;
            const value_type* operator->() const noexcept { return &val; }
        };


        iterator() noexcept : ptr_(nullptr), begin_(nullptr), end_(nullptr) {}
        iterator(slot_type* p, slot_type* b, slot_type* e, bool skip = false) noexcept
            : ptr_(p), begin_(b), end_(e)
        {
            if (skip) advance_to_valid();
        }

        iterator& operator++() noexcept
        {
            ++ptr_;
            advance_to_valid();
            return *this;
        }

        iterator operator++(int) noexcept
        {
            iterator tmp = *this;
            ++*this;
            return tmp;
        }

        iterator& operator--() noexcept
        {
            while (ptr_ != begin_)
            {
                --ptr_;
                if (ptr_->is_valid())
                    break;
            }
            return *this;
        }

        iterator operator--(int) noexcept
        {
            iterator tmp = *this;
            --*this;
            return tmp;
        }

        reference operator*() const noexcept
        {
            return { ptr_->key_, ptr_->value_ };
        }

        arrow_proxy operator->() const noexcept
        {
            return arrow_proxy{ { ptr_->key_, ptr_->value_ } };
        }

        bool operator==(const iterator& it) const noexcept { return ptr_ == it.ptr_; }
        bool operator!=(const iterator& it) const noexcept { return ptr_ != it.ptr_; }


    };


private:
    alignas(64) slot_type slots_[N]{};
    alignas(64) std::atomic<size_type> size_{0};
    [[no_unique_address]] Hash hash_fn_{};
    

public:

    static_assert(std::has_single_bit(N), "ConcurrentFixedFlatMap: N must be power-of-two.");

    static constexpr size_type capacity    = N;
    static constexpr size_type index_mask  = N - 1; 
    static constexpr size_type max_load    = N - 1;  

    ConcurrentFixedFlatMap() noexcept = default;
    ConcurrentFixedFlatMap(const ConcurrentFixedFlatMap&) = delete;
    ConcurrentFixedFlatMap& operator=(const ConcurrentFixedFlatMap&) = delete;

    std::pair<iterator, bool> insert(const K& key, const V& value) noexcept 
    {
        if (UNLIKELY(size_.load(std::memory_order_relaxed) >= max_load))
            return { end(), false };

        const size_type ideal = hash_fn_(key) & index_mask;
        size_type probe_idx   = ideal;

        for (size_type i = 0; i < N; ++i) 
        {
            slot_type& slot = slots_[probe_idx];

            if (slot.state_.load(std::memory_order_acquire) == SlotState::Empty) 
            {
                slot.insert_slot(key, value);
                size_.fetch_add(1, std::memory_order_release);
                return { iterator(&slot, slots_, slots_ + N), true };
            }
           
            probe_idx = (probe_idx + 1) & index_mask;
        }
        return { end(), false };
    }

    iterator find(const K& key) noexcept 
    {
        const size_type ideal = hash_fn_(key) & index_mask;
        size_type probe_idx   = ideal;

        for (size_type i = 0; i < N; ++i) 
        {
            const slot_type& slot = slots_[probe_idx];
            
            if (slot.is_valid()) 
            {
                if (slot.key_ == key)
                return iterator(slots_ + probe_idx, slots_, slots_ + N);
            }

            probe_idx = (probe_idx + 1) & index_mask;
        }

        return end();
    }

    bool erase(const K& key) noexcept 
    {
        const size_type ideal = hash_fn_(key) & index_mask;
        size_type probe_idx   = ideal;

        for (size_type i = 0; i < N; ++i) 
        {
            slot_type& slot = slots_[probe_idx];
            
            if (slot.is_valid()) 
            {
                if (slot.key_ == key) 
                {
                    slot.delete_slot();
                    size_.fetch_sub(1, std::memory_order_release);
                    return true;
                }
            }

            probe_idx = (probe_idx + 1) & index_mask;
        }
        return false;
    }

    void clear() noexcept 
    {
        for (auto& slot : slots_) slot.delete_slot();
        size_.store(0, std::memory_order_release);
    }

    //getters
    size_type size() const noexcept { return size_.load(std::memory_order_acquire); }
    bool empty() const noexcept { return size() == 0; }
    bool full() const noexcept { return size() >= max_load; }
    iterator begin() noexcept { return iterator(slots_, slots_, slots_ + N, true); }
    iterator end() noexcept { return iterator(slots_ + N, slots_, slots_ + N); }
};



