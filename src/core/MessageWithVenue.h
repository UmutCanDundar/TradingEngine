#include "Protocol-Venue.h"
#include "FIXMessage.h"
#include "ITCHTypes_BIST.h"
#include "ITCHTypes_NASDAQ.h"
#include "OUCHTypes_BIST.h"
#include "OUCHTypes_NASDAQ.h"

#include <variant>
#include <utility>
#include <cstddef>

#include <boost/lockfree/spsc_queue.hpp>

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
using spscMessageQueue_t = boost::lockfree::spsc_queue<MessageWithVenue<MessageTypes_t>, boost::lockfree::capacity<MESSAGE_QUEUE_CAPACITY>>;