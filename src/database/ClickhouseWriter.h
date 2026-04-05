// =============================================================================
// ClickHouseWriter (Implementation in progress. Some message types, init function 
//                   and optimizations are not finalized yet.)
//
// PURPOSE:
// - Consumes parsed trading messages from an SPSC queue.
// - Persists order and market events into ClickHouse for analytics,
//   backtesting, and post-trade analysis.
//
// THREADING MODEL:
// - Single-consumer worker.
// - Not thread-safe by design; external synchronization via SPSC queues.
//
// PERFORMANCE NOTES:
// - Cold path (non-latency-critical).
//
// DEVELOPER NOTES(PRE-PROFILING):
// - Current implementation performs row-by-row inserts.
// - Can be extended to batch multiple rows into a single ClickHouse Block
//   to improve throughput under high-volume or replay workloads.
// =============================================================================

#pragma once

#include "MessageWithVenue.h"
#include "Protocol-Venue.h"
#include "DbTypes.h"

#include <memory>
#include <string_view>
#include <string>

#include <clickhouse/client.h>

struct Order;

class ClickhouseWriter
{
private:
    std::unique_ptr<clickhouse::Client> client_;
    spscDbQueue_t &store_to_db_;
    spscDbQueue_t &db_to_parser_;

public:
    ClickhouseWriter(spscDbQueue_t &store_to_db, spscDbQueue_t &db_to_parser, const std::string &host = "127.0.0.1", int port = 9000) noexcept;

    bool store() noexcept;
   
private:
    void insert(const MessageWithVenue<BIST::ITCHMessage> &itchMsg);
    void insert(const MessageWithVenue<NASDAQ::ITCHMessage> &itchMsg);
    void insert(const MessageWithVenue<BIST::OUCHMessage> &ouchMsg);
    void insert(const MessageWithVenue<NASDAQ::OUCHMessage> &ouchMsg);
    void insert(const MessageWithVenue<FIXMessage *> &fixMsg);
    void insert(const MessageWithVenue<FIXSessionMessage *> &fixMsg);
    void insert(const Order *order);

    std::string_view venue_to_string(Venue venue) noexcept;
    std::string_view protocol_to_string(Protocol protocol) noexcept;
    std::string_view status_to_string(Status status) noexcept;
};
