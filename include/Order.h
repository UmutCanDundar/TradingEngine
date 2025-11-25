#pragma once

#include "Protocol-Venue.h"
#include <cstdint>
#include <atomic>
#include <array>
#include <vector>
#include <string_view>
#include <cstddef>

enum class SyncState : uint8_t 
{
    WaitingNew = 0,   // NEW henüz gelmedi
    NewSeen = 1,      // NEW (veya ACCEPT) görüldü
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
   Stop = 3,
   StopLimit = 4
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
   // === BASIC STATES ===
   Unknown = 0,   // İlk state, henüz bilgi yok
   New = 1,       // Order kabul edildi, market'ta aktif
   Partial = 2,   // Kısmen doldu, hala aktif
   Filled = 3,    // Tamamen doldu
   Cancelled = 4, // İptal edildi
   Replaced = 5,  // Değiştirildi (Modify)

   // === PENDING STATES ===
   PendingNew = 10,     // Order gönderildi, onay bekleniyor
   PendingCancel = 11,  // Cancel gönderildi, onay bekleniyor
   PendingReplace = 12, // Modify gönderildi, onay bekleniyor

   // === REJECTION STATES ===
   Rejected = 20,      // Order reddedildi
   CancelReject = 21,  // Cancel reddedildi
   ReplaceReject = 22, // Modify reddedildi

   // === EXCHANGE SPECIFIC ===
   Suspended = 30,                 // Trading halted
   Expired = 31,                   // TTL süresi doldu
   
   // === ERROR STATES ===
   DoneForDay = 40, // Gün sonu kapandı
   Calculated = 41, // Hesaplanmış ama henüz aktif değil
   Stopped = 42,    // Stop order tetiklendi

   // === SYSTEM STATES ===
   PendingSubmit = 50, // Sistem tarafında bekliyor
   Accepted = 51,      // Exchange kabul etti
   Restated = 52       // Order yeniden tanımlandı
};

inline constexpr size_t SYMBOL_SIZE = 32;
inline constexpr size_t ORDER_TOKEN_SIZE = 32;
inline constexpr size_t FIX_ORDER_ID_SIZE = 32;
inline constexpr size_t ORDER_IDs_SIZE = 32;

struct alignas(64) Order
{
   // 🔴 HOT PATH (en sık erişilen alanlar)
   int64_t price = 0;                                 // Fixed-point scaled
   uint32_t quantity = 0;                             // Original order quantity
   uint32_t filled_quantity = 0;                      // Cumulative filled quantity
   uint32_t remaining_quantity = 0;
   uint32_t last_exec_quantity = 0;
   int32_t replaced_quantity = 0;               // For Replace operations
   uint32_t symbol_index = 0;                         // From Hashtable 
   uint32_t instrument_id = 0;                        // SBE/ITCH/FIX instrument identifier
   Side side = Side::Unknown;                         // Buy/Sell
   Status status = Status::Unknown;                   // New/Partial/Filled/Cancelled
   Venue venue;                                       // NYSE, NASDAQ, etc.
   bool isOurOrder = false;       
   std::atomic<uint8_t> canModify = 0x00;             // Bitmask for allowed modifications
   Protocol protocol = Protocol::Unknown;             // Source protocol for reconstruction
   OrderType order_type = OrderType::Unknown;         // Limit, Market, Stop
   TimeInForce time_in_force = TimeInForce::Unknown;  // IOC, GTC, etc.
   uint8_t pad1[4];                                  // 64-byte alignment
   
   // 🟠 LOOKUP & ROUTING
   uint64_t order_id = 0;         // Exchange-assigned unique order ID (ITCH and Hash-based FIX)
   uint64_t client_order_id = 0;  // Hash_based rclient_order_token
   
   // Cache-Line
   uint64_t timestamp = 0;                      // Order creation time 
   uint64_t last_update_time = 0;                // Last modification time
   uint8_t cancelled_count = 0;
   SyncState syncState = SyncState::WaitingNew; // For FIX
   std::array<Status, 2> StatusesPreNew;        // For FIX
   uint8_t exec_type = 0;                       // For FIX
  
   // 🟡 PROTOCOL - SYMBOL DATA for builder
   uint8_t message_type = 0;                                // Message type within the protocol
   std::array<char, SYMBOL_SIZE> symbol{};                  // Fixed-size symbol for low-latency lookup
   uint8_t pad2[10]; // 64-byte alignment
   
   //Cache-Line
   std::array<char, FIX_ORDER_ID_SIZE> fix_org_order_id{};  // For FIX original order ID (This field may be used to hold the existing-order-token for ouch replaced messages)
   std::array<char, ORDER_TOKEN_SIZE> client_order_token{}; // Unique client order token
   
};