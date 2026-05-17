// =============================================================================
// Order
//
// PURPOSE:
// - Core order representation shared across the whole system.
// - Designed as a low-latency, data-oriented carrier for order lifecycle state.
//
// THREAD SAFETY:
// - Single-writer / single-reader (SPSC) ownership model.
// - Immutable across threads except for `canModify`, which is atomic.
// - Safe for concurrent access under the defined ownership guarantees.
//
// PERFORMANCE CHARACTERISTICS:
// - Hot-path structure; optimized for cache efficiency.
// - No internal locking; relies on external SPSC synchronization.
//
// DEVELOPER NOTES(PRE-PROFILING):
// - Should be passed by pointer/reference, not by value.
// - Avoid large stack allocations of Order arrays.
// - After instruction-cache and data-cache profiling,
//   OrderHot / OrderCold separation may be introduced
//   to improve cache locality on matching and risk hot-paths.
// - SyncState mechanism for FIX is kept temporarily for safety, 
//   but will be removed in production since strict message ordering is guaranteed 
//   at the parser layer
//
// =============================================================================

#pragma once

#include "Protocol-Venue.h"

#include <cstdint>
#include <atomic>
#include <array>
#include <cstddef>

#include <boost/lockfree/spsc_queue.hpp>

enum class SyncState : uint8_t 
{
    WaitingNew = 0,   
    NewSeen = 1,
};

enum class Side : uint8_t
{
   Buy = 0,
   Sell = 1,
   Unknown = 2,
};

enum class OrderType : uint8_t
{
   Unknown = 0,
   Market = 1,
   Limit = 2,
};

enum class TimeInForce : uint8_t
{
   DAY = 0,
   GTC = 1,
   IOC = 3,
   FOK = 4,
   GTD = 6,
   AT_CROSSING = 9,
   GTS = 'S',
   Unknown = 99
};

enum class Status : uint8_t
{
   Unknown = 0,  
   New = 1,       
   Partial = 2,   
   Filled = 3,    
   Cancelled = 4, 
   Replaced = 5,
   Deleted = 6, 
   Expired = 7,
   CancelRequest = 8,

};

inline constexpr size_t SYMBOL_SIZE = 32;
inline constexpr size_t ORDER_TOKEN_SIZE = 32;
inline constexpr size_t FIX_ORDER_ID_SIZE = 32;
inline constexpr size_t ORDER_IDs_SIZE = 32;

inline constexpr uint8_t STRATEGY_DONE = 0x01;
inline constexpr uint8_t RISK_DONE = 0x02;
inline constexpr uint8_t BUILDER_DONE = 0x03;

struct alignas(64) Order // 192B
{
   // 🔴 HOT DATA 
   int64_t price = 0;                                 // Fixed-point scaled
   uint32_t quantity = 0;                             
   uint32_t filled_quantity = 0;                      // Cumulative filled quantity
   uint32_t remaining_quantity = 0;
   uint32_t last_exec_quantity = 0;
   int32_t replaced_quantity = 0;                    
   uint32_t symbol_index = 0;                         // From Hashtable 
   uint32_t instrument_id = 0;                        
   uint32_t user_ref_num = 0;                         // For OUCH-NASDAQ 
   Side side = Side::Unknown;                        
   Status status = Status::Unknown;                   
   Venue venue;                                       
   bool isOurOrder = false;       
   std::atomic<uint8_t> canModify = STRATEGY_DONE | RISK_DONE | BUILDER_DONE;         // Bitmask for allowed modifications
   Protocol protocol = Protocol::Unknown;             
   OrderType order_type = OrderType::Unknown;         
   TimeInForce time_in_force = TimeInForce::Unknown;  
   
   // 🟠 LOOKUP & ROUTING
   uint64_t order_id = 0;         // Exchange-assigned unique order ID (ITCH and Hash-based FIX)
   uint64_t client_order_id = 0;  // Hash_based rclient_order_token
   // Cache-Line 1
   
   uint64_t timestamp = 0;                     
   uint64_t last_update_time = 0;
   uint32_t pre_user_ref_num = 0; // For OUCH-NASDAQ
   uint8_t cancelled_count = 0;
   SyncState syncState = SyncState::WaitingNew;    // For FIX (In case a New Order message is received after an acknowledgment message.)
   std::array<Status, 2> StatusesPreNew;        // For FIX
   uint8_t exec_type = 0;                       // For FIX
  
   // 🟡 PROTOCOL - SYMBOL - ACCOUNT DATA for builder
   uint8_t message_type = 0;
   uint8_t account_index = 0;                               
   std::array<char, SYMBOL_SIZE> symbol{};
   uint8_t real_symbol_len = 0; 
   uint8_t real_cl_ord_token_len = 0; 

   uint8_t pad[3]; 
   //Cache-Line 2
   
   std::array<char, FIX_ORDER_ID_SIZE> fix_org_order_id{};  // For FIX original order ID (This field may be used to hold the existing-order-token for ouch replaced messages)
   std::array<char, ORDER_TOKEN_SIZE> client_order_token{}; // Unique client order token
   //Cache-Line 3

   Order() noexcept = default;
   Order(const Order&) = delete;
   Order(Order&&) = delete;
   Order& operator=(const Order&) = delete;
   Order& operator=(Order&&) = delete;
};

inline constexpr size_t ORDER_QUEUE_CAPACITY = 1024;
using spscOrderQueue_t = boost::lockfree::spsc_queue<Order *, boost::lockfree::capacity<ORDER_QUEUE_CAPACITY>>;
