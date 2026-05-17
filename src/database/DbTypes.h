#include "MessageWithVenue.h"
#include "Order.h"

#include <variant>
#include <cstddef>

#include <boost/lockfree/spsc_queue.hpp>

#pragma once

inline constexpr size_t DB_QUEUE_CAPACITY = 1024;

using DbData_t = std::variant<Order *, MessageWithVenue<FIXMessage *>, MessageWithVenue<FIXSessionMessage *>, MessageWithVenue<BIST::ITCHMessage>, MessageWithVenue<BIST::OUCHMessage>, MessageWithVenue<NASDAQ::ITCHMessage>, MessageWithVenue<NASDAQ::OUCHMessage>>;
using spscDbQueue_t = boost::lockfree::spsc_queue<std::variant<Order *, MessageWithVenue<FIXMessage *>, MessageWithVenue<FIXSessionMessage *>, MessageWithVenue<BIST::ITCHMessage>, MessageWithVenue<BIST::OUCHMessage>, MessageWithVenue<NASDAQ::ITCHMessage>, MessageWithVenue<NASDAQ::OUCHMessage>>, boost::lockfree::capacity<DB_QUEUE_CAPACITY>>;
