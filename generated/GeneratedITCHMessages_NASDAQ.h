// Generated automatically. DO NOT EDIT!
// NASDAQ ITCH Messages Struct Definitions
#pragma once

#include <cstdint>

namespace NASDAQ {

struct ITCHAddOrderMessage {
    uint64_t timestamp = 0;              // Message time (nanoseconds)  offset: 5 | len: 6
    uint64_t order_ref = 0;              // Order reference number (unique)  offset: 11 | len: 8
    uint32_t shares = 0;                 // Order quantity (shares)  offset: 20 | len: 4
    uint32_t price = 0;                  // Price (in 1/10000 USD, integer)  offset: 32 | len: 4
    uint16_t stock_locate = 0;           // Stock ID (Stock Locate)  offset: 1 | len: 2
    uint16_t tracking_number = 0;        // Message tracking number  offset: 3 | len: 2
    char message_type = 'A';             // Message Type (A = Add Order)  offset: 0 | len: 1
    char side = '\0';                    // Order side ('B' = Buy, 'S' = Sell)  offset: 19 | len: 1
    char stock[8] = {};                  // Stock symbol (8 characters, space-padded)  offset: 24 | len: 8
};

struct ITCHAddOrderMPIDMessage {
    uint64_t timestamp = 0;              // Message time (nanoseconds) | offset: 5, len: 6
    uint64_t order_ref = 0;              // Order reference number | offset: 11, len: 8
    uint32_t shares = 0;                 // Order quantity | offset: 20, len: 4
    uint32_t price = 0;                  // Price | offset: 32, len: 4
    uint16_t stock_locate = 0;           // Stock ID | offset: 1, len: 2
    uint16_t tracking_number = 0;        // Tracking number | offset: 3, len: 2
    char message_type = 'F';             // Message Type (F = Add Order with MPID) | offset: 0, len: 1
    char side = '\0';                    // Order side ('B'=Buy, 'S'=Sell) | offset: 19, len: 1
    char stock[8] = {};                  // Stock symbol | offset: 24, len: 8
    char mpid[4] = {};                   // Market Participant ID (MPID) | offset: 36, len: 4
};

struct ITCHCancelMessage {
    uint64_t timestamp = 0;               // Cancel time | offset: 5, len: 6
    uint64_t order_ref = 0;               // Cancelled order reference | offset: 11, len: 8
    uint32_t cancelled_shares = 0;        // Cancelled quantity | offset: 19, len: 4
    uint16_t stock_locate = 0;            // Stock ID | offset: 1, len: 2
    uint16_t tracking_number = 0;         // Tracking number | offset: 3, len: 2
    char message_type = 'X';              // Message Type (X = Order Cancel) | offset: 0, len: 1
};

struct ITCHExecutedMessage {
    uint64_t timestamp = 0;              // Execution time | offset: 5, len: 6
    uint64_t order_ref = 0;              // Executed order reference | offset: 11, len: 8
    uint64_t match_number = 0;           // Match number | offset: 23, len: 8
    uint32_t executed_shares = 0;        // Executed quantity | offset: 19, len: 4
    uint16_t stock_locate = 0;           // Stock ID | offset: 1, len: 2
    uint16_t tracking_number = 0;        // Tracking number | offset: 3, len: 2
    char message_type = 'E';             // Message Type (E = Order Executed, no price) | offset: 0, len: 1
};

struct ITCHExecutedWithPriceMessage {
    uint64_t timestamp = 0;              // Execution time | offset: 5, len: 6
    uint64_t order_ref = 0;              // Executed order reference | offset: 11, len: 8
    uint64_t match_number = 0;           // Match number | offset: 23, len: 8
    uint32_t executed_shares = 0;        // Executed quantity | offset: 19, len: 4
    uint32_t execution_price = 0;        // Execution price | offset: 32, len: 4
    uint16_t stock_locate = 0;           // Stock ID | offset: 1, len: 2
    uint16_t tracking_number = 0;        // Tracking number | offset: 3, len: 2
    char message_type = 'C';             // Message Type (C = Order Executed with price) | offset: 0, len: 1
    char printable = '\0';               // Price printable status (Y/N) | offset: 31, len: 1
};

struct ITCHDeleteMessage {
    uint64_t timestamp = 0;              // Delete time | offset: 5, len: 6
    uint64_t order_ref = 0;              // Deleted order reference | offset: 11, len: 8
    uint16_t stock_locate = 0;           // Stock ID | offset: 1, len: 2
    uint16_t tracking_number = 0;        // Tracking number | offset: 3, len: 2
    char message_type = 'D';             // Message Type (D = Order Deleted) | offset: 0, len: 1
};

struct ITCHReplaceMessage {
    uint64_t timestamp = 0;              // Message time (nanoseconds)  offset: 5 | len: 6
    uint64_t order_ref = 0;              // Order reference number (unique)  offset: 11 | len: 8
    uint64_t new_order_ref = 0;          // New order reference number (unique)  offset: 19 | len: 8
    uint32_t shares = 0;                 // Order quantity (shares)  offset: 27 | len: 4
    uint32_t price = 0;                  // Price (in 1/10000 USD, integer)  offset: 31 | len: 4
    uint16_t stock_locate = 0;           // Stock ID (Stock Locate)  offset: 1 | len: 2
    uint16_t tracking_number = 0;        // Message tracking number  offset: 3 | len: 2
    char message_type = 'U';             // Message Type (U = Order Replace)  offset: 0 | len: 1
};

struct ITCHTradeMessage {
    uint64_t timestamp = 0;              // Trade time | offset: 5, len: 6
    uint64_t order_ref = 0;              // Related order reference | offset: 11, len: 8
    uint64_t match_number = 0;           // Match number | offset: 28, len: 8
    uint32_t shares = 0;                 // Trade quantity | offset: 20, len: 4
    uint32_t price = 0;                  // Trade price | offset: 24, len: 4
    uint16_t stock_locate = 0;           // Stock ID | offset: 1, len: 2
    uint16_t tracking_number = 0;        // Tracking number | offset: 3, len: 2
    char message_type = 'P';             // Message Type (P = Trade) | offset: 0, len: 1
    char side = '\0';                    // Trade side ('B'/'S') | offset: 19, len: 1
    char cross_type = '\0';              // Cross type | offset: 36, len: 1
};

struct ITCHSystemEventMessage {
    uint64_t timestamp = 0;              // Event time | offset: 5, len: 6
    uint16_t stock_locate = 0;           // Stock ID (always 0) | offset: 1, len: 2
    uint16_t tracking_number = 0;        // Tracking number | offset: 3, len: 2
    char message_type = 'S';             // Message Type (S = System Event) | offset: 0, len: 1
    char event_code = '\0';              // Event code | offset: 11, len: 1
};

struct ITCHStockDirectoryMessage {
    uint64_t timestamp = 0;                            // Event time | offset: 5, len: 6
    uint32_t round_lot_size = 0;                       // Round lot size | offset: 21, len: 4
    uint32_t etp_leverage_factor = 0;                  // ETP leverage | offset: 34, len: 4
    uint16_t stock_locate = 0;                         // Stock ID | offset: 1, len: 2
    uint16_t tracking_number = 0;                      // Tracking number | offset: 3, len: 2
    char message_type = 'R';                           // Message Type (R = Stock Directory) | offset: 0, len: 1
    char stock[8] = {};                                // Stock symbol | offset: 11, len: 8
    char market_category = '\0';                       // Market category | offset: 19, len: 1
    char financial_status_indicator = '\0';            // Financial status code | offset: 20, len: 1
    char round_lots_only = '\0';                       // Round lots only | offset: 25, len: 1
    char issue_classification = '\0';                  // Issue classification | offset: 26, len: 1
    char issue_sub_type[2] = {};                       // Issue sub-type | offset: 27, len: 2
    char authenticity = '\0';                          // Authenticity code | offset: 29, len: 1
    char short_sale_threshold_indicator = '\0';        // Short sale threshold | offset: 30, len: 1
    char ipo_flag = '\0';                              // IPO flag | offset: 31, len: 1
    char luld_reference_price_tier = '\0';             // LULD price tier | offset: 32, len: 1
    char etp_flag = '\0';                              // ETP flag | offset: 33, len: 1
    char inverse_indicator = '\0';                     // Inverse indicator | offset: 38, len: 1
};

struct ITCHTradingStateMessage {
    uint64_t timestamp = 0;              // Event time | offset: 5, len: 6
    uint16_t stock_locate = 0;           // Stock ID | offset: 1, len: 2
    uint16_t tracking_number = 0;        // Tracking number | offset: 3, len: 2
    char message_type = 'H';             // Message Type (H = Trading State) | offset: 0, len: 1
    char stock[8] = {};                  // Stock symbol | offset: 11, len: 8
    char trading_state = 'T';            // T=Trading, H=Halted, P=Paused, Q=Quotation | offset: 19, len: 1
    char reserved = '\0';                // Reserved | offset: 20, len: 1
    char reason[4] = {};                 // Reason | offset: 21, len: 4
};

enum class ITCHTypes : uint8_t {   A = 0,  F = 1,  X = 2,  E = 3,  C = 4,  D = 5,  U = 6,  P = 7,  S = 8,  R = 9,  H = 10,  unknownITCHtype = 99 };

inline constexpr ITCHTypes itchMessageIndex(char type) {

   switch(type) {
      case 'A': return ITCHTypes::A;
      case 'F': return ITCHTypes::F;
      case 'X': return ITCHTypes::X;
      case 'E': return ITCHTypes::E;
      case 'C': return ITCHTypes::C;
      case 'D': return ITCHTypes::D;
      case 'U': return ITCHTypes::U;
      case 'P': return ITCHTypes::P;
      case 'S': return ITCHTypes::S;
      case 'R': return ITCHTypes::R;
      case 'H': return ITCHTypes::H;
      default: return ITCHTypes::unknownITCHtype;
   }
}

}
