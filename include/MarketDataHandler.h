#ifndef MARKETDATAHANDLER_H
#define MARKETDATAHANDLER_H

#pragma once

#include "MulticastReceiver.h"
#include "common.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <string_view>

// #include "readerwriterqueue.h"  // moodycamel queue

struct alignas(64) MarketData
{

    uint64_t sequence_num;   // message/order sequence
    uint64_t timestamp_ns;   // nanosecond-level timestamp
    int64_t price_int;       // price * 10000 (fixed point)
    uint32_t quantity;       // order size
    uint32_t symbol_id;      // 4 byte
    uint8_t exchange_id;     // 1 byte
    uint8_t side;            // 1 byte
    uint8_t trade_condition; // 1 byte
    uint8_t exchange_code;   // e.g., 'X' = NASDAQ
};

class MarketDataHandler
{
private:
    MulticastReceiver receiver_;
    // FixParser<MarketDataHandler> parser_;
    // SharedMemoryStore store_;
public:
};

#endif