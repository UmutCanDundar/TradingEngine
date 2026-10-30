#include "MessageWithVenue.h"
#include "Order.h"

#include <variant>
#include <cstddef>

#include <boost/lockfree/spsc_queue.hpp>
#include "CustomSpscQueue.h"


#pragma once


inline constexpr size_t DB_QUEUE_CAPACITY = 1024;

using DbData_t = std::variant<Order *, MessageWithVenue<FIXMessage *>*, MessageWithVenue<FIXSessionMessage *>*, MessageWithVenue<BIST::ITCHMessage>*, MessageWithVenue<BIST::OUCHMessage>*, MessageWithVenue<NASDAQ::ITCHMessage>*, MessageWithVenue<NASDAQ::OUCHMessage>*>;

struct DbData
{
    DbData_t data;
};

// using spscDbQueue_t = boost::lockfree::spsc_queue<DbData*, boost::lockfree::capacity<DB_QUEUE_CAPACITY>>;
using spscDbQueue_t = CustomSpscQueue<DbData*, DB_QUEUE_CAPACITY>;



class DbData_PoolManager
{
private:
    static constexpr size_t DBDATA_POOL_CAPACITY = 2048;

    std::array<DbData, DBDATA_POOL_CAPACITY> pool{};
    size_t next_DbData{0};

public:
    DbData* get_DbData(DbData_t data) noexcept
    {
        auto* dbd = &pool[next_DbData++ & (DBDATA_POOL_CAPACITY - 1)];
        dbd->data = data;

        return dbd; 
    }

};
