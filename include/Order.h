#pragma once

#include "Protocol-Venue.h"
#include <cstdint>
#include <atomic>
#include <array>

enum class Side : uint8_t
{
   Buy = 0,
   Sell = 1,
   Unknown = 2,
};
enum class Status : uint8_t
{
   // === BASIC STATES ===
   Unknown = 0,   // İlk state, henüz bilgi yok
   New = 1,       // Order kabul edildi, market'ta aktif
   Partial = 2,   // Kısmen doldu, hala aktif
   Filled = 3,    // Tamamen doldu
   Cancelled = 4, // İptal edildi

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
   PartiallyFilled_Cancelled = 32, // Kısmen dolup iptal edildi

   // === ERROR STATES ===
   DoneForDay = 40, // Gün sonu kapandı
   Calculated = 41, // Hesaplanmış ama henüz aktif değil
   Stopped = 42,    // Stop order tetiklendi

   // === SYSTEM STATES ===
   PendingSubmit = 50, // Sistem tarafında bekliyor
   Accepted = 51,      // Exchange kabul etti
   Restated = 52       // Order yeniden tanımlandı
};

struct alignas(64) Order
{
   // 🔴 HOT PATH (en sık erişilen alanlar)
   int64_t price = 0;            // Fixed-point scaled
   uint32_t quantity = 0;        // Original order quantity
   uint32_t filled_quantity = 0; // Cumulative filled quantity
   uint32_t cancelled_quantity = 0;
   uint32_t last_exec_quantity = 0;
   std::array<char, 8> symbol{};    // Fixed-size symbol for low-latency lookup
   Side side = Side::Unknown;       // Buy/Sell
   Status status = Status::Unknown; // New/Partial/Filled/Cancelled

   // 🟠 LOOKUP & ROUTING
   Venue venue; // NYSE, NASDAQ, etc.
   bool isOurOrder = false;
   std::atomic<uint8_t> canModify = 0x00;
   uint8_t pad1[3];
   uint64_t order_id = 0;        // Exchange-assigned unique order ID
   uint64_t client_order_id = 0; // Strategy-assigned client order ID
   uint64_t timestamp = 0;       // Order creation time (ns epoch)
   // Cache-Line
   uint64_t last_update_time = 0; // Last modification time (ns epoch)

   // 🟡 PROTOCOL METADATA
   uint16_t message_type = 0;             // FIX char, ITCH char, SBE templateId
   Protocol protocol = Protocol::Unknown; // Source protocol for reconstruction

   // 🟢 OPTIONAL (protokol bazlı karar & advanced tactics)
   uint8_t time_in_force = 0;  // IOC, GTC, etc.
   uint32_t instrument_id = 0; // SBE/ITCH
   uint8_t order_type = 0;     // Limit, Market, Stop
   uint8_t priority_level = 0; // HFT queue tactics

   uint8_t pad2[46]; // 64-byte alignment
};