#include "Protocol-Venue.h"
#include "FIXMessage.h"
#include "ITCHTypes_BIST.h"
#include "ITCHTypes_NASDAQ.h"
#include "OUCHTypes_BIST.h"
#include "OUCHTypes_NASDAQ.h"

#include <variant>
#include <utility>
#include <cstddef>
#include <type_traits>

#include <boost/lockfree/spsc_queue.hpp>
#include "CustomSpscQueue.h"

#pragma once

template <typename T>
struct MessageWithVenue
{
    T msg;
    Venue venue;

    MessageWithVenue() noexcept = default;
    MessageWithVenue(T m, Venue ven) noexcept : msg(m), venue(ven) {}
};

inline constexpr size_t MESSAGE_QUEUE_CAPACITY = 1024;

using MessageTypes_t = std::variant<FIXMessage *, FIXSessionMessage *, BIST::ITCHMessage, NASDAQ::ITCHMessage, BIST::OUCHMessage, NASDAQ::OUCHMessage>;
// using spscMessageQueue_t = boost::lockfree::spsc_queue<MessageWithVenue<MessageTypes_t>*, boost::lockfree::capacity<MESSAGE_QUEUE_CAPACITY>>;
using spscMessageQueue_t = CustomSpscQueue<MessageWithVenue<MessageTypes_t>*, MESSAGE_QUEUE_CAPACITY>;

template <typename T>
class MessageWithVenuePool
{
private:
    static constexpr size_t MESSAGE_POOL_CAPACITY = std::is_same_v<T, MessageTypes_t> ? 2048 : 256;

    std::array<MessageWithVenue<T>, MESSAGE_POOL_CAPACITY> pool{};
    size_t next_msgWithVenue{0};

public:
    MessageWithVenue<T>* get_msgWithVenue(T* msg, Venue venue) noexcept
    {
        auto* mwv = &pool[next_msgWithVenue++ & (MESSAGE_POOL_CAPACITY - 1)];
        mwv->msg = msg;
        mwv->venue = venue;
        return mwv; 
    }

    MessageWithVenue<T>* get_msgWithVenue(T&& msg, Venue venue) noexcept
    {
        auto* mwv = &pool[next_msgWithVenue++ & (MESSAGE_POOL_CAPACITY - 1)];
        mwv->msg = std::forward<T>(msg);
        mwv->venue = venue;
        return mwv; 
    }

};


template <typename ...Ts>
class MessageWithVenue_PoolManager
{
private:
    std::tuple<MessageWithVenuePool<Ts>...> message_pools_;
        
public:

    template <typename T>
    MessageWithVenue<T>* get_msgWithVenue(T* msg = nullptr, Venue venue = Venue::BIST) noexcept
    {
        return std::get<MessageWithVenuePool<T>>(message_pools_).get_msgWithVenue(msg, venue);
    }

    template <typename T>
    MessageWithVenue<T>* get_msgWithVenue(T&& msg, Venue venue = Venue::BIST) noexcept
    {
        return std::get<MessageWithVenuePool<T>>(message_pools_).get_msgWithVenue(std::forward<T>(msg), venue);
    }

};

using MWV_PoolManager_t = MessageWithVenue_PoolManager<
    MessageTypes_t, 
    FIXMessage *, 
    FIXSessionMessage *, 
    BIST::ITCHMessage, 
    NASDAQ::ITCHMessage, 
    BIST::OUCHMessage, 
    NASDAQ::OUCHMessage
>;