#pragma once

#include <tuple>
#include <boost/pool/object_pool.hpp>
#include <boost/lockfree/spsc_queue.hpp>

// Her mesaj türü için kullanılacak havuz yapısı
template <typename T>
struct PoolAndFreeList
{
    // Capacity sabit olsun
    static constexpr size_t FREELIST_CAPACITY = 1024;

    boost::object_pool<T> pool_;
    boost::lockfree::spsc_queue<T *> free_list_{FREELIST_CAPACITY};

    void initialize()
    {
        for (size_t i = 0; i < FREELIST_CAPACITY; ++i)
            free_list_.push(pool_.construct());
    }

    void acquire(T *&obj) noexcept
    {
        free_list_.pop(obj);
    }

    void release(T *&obj) noexcept
    {
        free_list_.push(obj);
    }
};

// Tuple içindeki tüm türler için Pool'ları tek yapıda birleştirir
template <typename T>
struct MessagePools;

template <typename... MsgTypes>
struct MessagePools<std::tuple<MsgTypes...>>
{
    std::tuple<PoolAndFreeList<MsgTypes>...> pool_and_freelist_;

    // Bütün türler için initialize çağrılır
    void initialize_all()
    {
        (std::get<PoolAndFreeList<MsgTypes>>(pool_and_freelist_).initialize(), ...);
    }

    // Tür'e özel pool'a erişim
    template <typename T>
    PoolAndFreeList<T> &get_pool() noexcept
    {
        return std::get<PoolAndFreeList<T>>(pool_and_freelist_);
    }
};
