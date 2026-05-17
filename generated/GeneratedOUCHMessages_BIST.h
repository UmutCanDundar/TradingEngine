// Generated automatically. DO NOT EDIT!
// BIST OUCH Messages Struct Definitions
#pragma once

#include <cstdint>

namespace BIST {

/* namespace TX {

struct OUCHEnterOrderMessage {
    uint64_t quantity = 0;                // Order quantity | offset: 20, len: 8
    uint64_t display_quantity = 0;        // Displayed qty (0=unchanged) | offset: 97, len: 8
    uint32_t order_book_id = 0;           // Order book identifier | offset: 15, len: 4
    int32_t price = 0;                    // Signed price | offset: 28, len: 4
    char message_type = 'O';              // Enter Order Message | offset: 0, len: 1
    char order_token[14] = {};            // Client-generated order identifier | offset: 1, len: 14
    char side = '\0';                     // Side (B=Buy, S=Sell) | offset: 19, len: 1
    uint8_t time_in_force = 0;            // 0=Day, 3=IOC, 4=FOK | offset: 32, len: 1
    uint8_t open_close = 0;               // 0=Default, 1=Open, 2=Close | offset: 33, len: 1
    char client_account[16] = {};         // Client/Account (AFK code) for DM | offset: 34, len: 16
    uint8_t client_category = 0;          // 1=Client,2=House,7=Fund for EQM | offset: 105, len: 1
    uint8_t offhours = 0;                 // 0=normal,1=off for DM | offset: 106, len: 1
    uint8_t smp_level = 0;                // scope of SMP | offset: 107, len: 1
    uint8_t smp_method = 0;               // the action being undertaken by the trading system in case of a potential prevention | offset: 108, len: 1
    char reserverd[2];                    // Reserved | offset: 112, len: 2
    char customer_info[15] = {};          // Client reference | offset: 50, len: 15
    char exchange_info[32] = {};          // Client account number/only first 16 bytes for EQM | offset: 65, len: 32
    char smp_ID[3] = {};                  // the order is eligible for self-match prevention | offset: 109, len: 3
};

struct OUCHReplaceOrderMessage {
    uint64_t quantity = 0;                        // Desired open quantity | offset: 29, len: 8
    uint64_t display_quantity = 0;                // Displayed qty (0=unchanged) | offset: 105, len: 8
    int32_t price = 0;                            // New price (0=no change) | offset: 37, len: 4
    char message_type = 'u';                      // Replace Order Message | offset: 0, len: 1
    char existing_order_token[14] = {};           // Existing order token | offset: 1, len: 14
    char replacement_order_token[14] = {};        // Replacement order token | offset: 15, len: 14
    char customer_info[15] = {};                  // Pass-thru field | offset: 58, len: 15
    uint8_t open_close = 0;                       // 0=No change,1=Open,2=Close | offset: 41, len: 1
    char client_account[16] = {};                 // Client/Account for DM | offset: 42, len: 16
    char exchange_info[32] = {};                  // Account number (first 16 used) for EQM | offset: 73, len: 32
    uint8_t client_category = 0;                  // 1=Client,2=House,7=Fund for EQM | offset: 113, len: 1
    char reserved[8];                             // Reserved | offset: 114, len: 8
};

struct OUCHCancelOrderMessage {
    char message_type = 'X';          // Cancel Order Message | offset: 0, len: 1
    char order_token[14] = {};        // Order token to cancel | offset: 1, len: 14
};

struct OUCHCancelByOrderIDMessage {
    uint64_t order_id = 0;             // Exchange-assigned ID | offset: 6, len: 8
    uint32_t order_book_id = 0;        // Order book identifier | offset: 1, len: 4
    char message_type = 'Y';           // Cancel Order Message | offset: 0, len: 1
    char side = '\0';                  // Side (B=Buy, S=Sell) | offset: 5, len: 1
};

} */ 
namespace RX
{

struct OUCHOrderAcceptedMessage {
    uint64_t timestamp = 0;              // UNIX ns timestamp | offset: 1, len: 8
    uint64_t order_id = 0;               // Exchange-assigned ID | offset: 28, len: 8
    uint64_t quantity = 0;               // Open quantity | offset: 36, len: 8
    uint64_t pre_trade_qty = 0;          // Pre-trade quantity | offset: 114, len: 8
    uint64_t display_qty = 0;            // Displayed quantity | offset: 122, len: 8
    uint32_t order_book_id = 0;          // Book ID | offset: 23, len: 4
    int32_t price = 0;                   // Price | offset: 44, len: 4
    char message_type = 'A';             // Order Accepted | offset: 0, len: 1
    char order_token[14] = {};           // Echo of client token | offset: 9, len: 14
    char side = '\0';                    // B/S/T | offset: 27, len: 1
    uint8_t time_in_force = 0;           // TIF | offset: 48, len: 1
    char client_account[16] = {};        // Account info for DM | offset: 50, len: 16
    char customer_info[15] = {};         // Pass-thru | offset: 67, len: 15
    char exchange_info[32] = {};         // Account info for EQM | offset: 82, len: 32
    uint8_t open_close = 0;              // Open/Close flag | offset: 49, len: 1
    uint8_t order_state = 0;             // 1=OnBook,2=NotOnBook,98=Paused | offset: 66, len: 1
    uint8_t client_category = 0;         // 1=Client,2=House,7=Fund for EQM | offset: 130, len: 1
    uint8_t offhours = 0;                // 0=normal,1=off for DM | offset: 131, len: 1
    uint8_t smp_level = 0;               // scope of SMP | offset: 132, len: 1
    uint8_t smp_method = 0;              // the action being undertaken by the trading system in case of a potential prevention | offset: 133, len: 1
    char smp_ID[3] = {};                 // the order is eligible for self-match prevention | offset: 134, len: 3
};

struct OUCHOrderRejectedMessage {
    uint64_t timestamp = 0;           // UNIX ns timestamp | offset: 1, len: 8
    int32_t reject_code = 0;          // Signed reason code | offset: 23, len: 4
    char message_type = 'J';          // Order Rejected | offset: 0, len: 1
    char order_token[14] = {};        // Echo of token | offset: 9, len: 14
};

struct OUCHOrderReplacedMessage {
    uint64_t timestamp = 0;                    // UNIX ns timestamp | offset: 1, len: 8
    uint64_t order_id = 0;                     // New order ID | offset: 42, len: 8
    uint64_t quantity = 0;                     // Open quantity | offset: 50, len: 8
    uint64_t pre_trade_qty = 0;                // Pre-trade quantity | offset: 128, len: 8
    uint32_t order_book_id = 0;                // Order book ID | offset: 37, len: 4
    int32_t price = 0;                         // Signed price | offset: 58, len: 4
    char message_type = 'U';                   // Order Replaced | offset: 0, len: 1
    char previous_order_token[14] = {};        // Previous Order Token | offset: 23, len: 14
    char side = '\0';                          // Side (B/S) | offset: 41, len: 1
    uint8_t time_in_force = 0;                 // Time-in-Force | offset: 62, len: 1
    uint8_t open_close = 0;                    // 0=No change,1=Open,2=Close,4=Default | offset: 63, len: 1
    uint8_t order_state = 0;                   // 1=On book,2=Not on book,98=Paused,99=Ownership lost | offset: 80, len: 1
    uint8_t client_category = 0;               // 1=Client,2=House,7=Fund for EQM | offset: 144, len: 1
    char order_token[14] = {};                 // Replacement Order Token | offset: 9, len: 14
    char exchange_info[32] = {};               // Account info for EQM | offset: 96, len: 32
    char client_account[16] = {};              // Client/Account (Derivatives) | offset: 64, len: 16
    char customer_info[15] = {};               // Pass-through | offset: 81, len: 15
    uint64_t display_qty = 0;                  // Displayed quantity | offset: 136, len: 8
};

struct OUCHOrderCancelledMessage {
    uint64_t timestamp = 0;            // UNIX ns timestamp | offset: 1, len: 8
    uint64_t order_id = 0;             // Canceled order ID | offset: 28, len: 8
    uint32_t order_book_id = 0;        // Order book ID | offset: 23, len: 4
    char message_type = 'C';           // Order Canceled | offset: 0, len: 1
    char order_token[14] = {};         // Order Token | offset: 9, len: 14
    char side = '\0';                  // Side (B/S) | offset: 27, len: 1
    uint8_t reason = 0;                // Cancel reason code | offset: 36, len: 1
};

struct OUCHOrderExecutedMessage {
    uint64_t timestamp = 0;              // UNIX ns timestamp | offset: 1, len: 8
    uint64_t traded_quantity = 0;        // Traded quantity | offset: 27, len: 8
    uint32_t order_book_id = 0;          // Order book ID | offset: 23, len: 4
    int32_t trade_price = 0;             // Trade price | offset: 35, len: 4
    char message_type = 'E';             // Order Executed | offset: 0, len: 1
    char order_token[14] = {};           // Order Token | offset: 9, len: 14
    char match_id[12] = {};              // Match ID | offset: 39, len: 12
    uint8_t client_category = 0;         // Client category | offset: 51, len: 1
    char reserved[16] = {};              // Reserved | offset: 52, len: 16
};

}

enum class OUCHTypes : uint8_t {   A = 0,  J = 1,  U = 2,  C = 3,  E = 4,  unknownOUCHtype = 99 };

inline constexpr OUCHTypes ouchMessageIndex(char type) {

   switch(type) {
      case 'A': return OUCHTypes::A;
      case 'J': return OUCHTypes::J;
      case 'U': return OUCHTypes::U;
      case 'C': return OUCHTypes::C;
      case 'E': return OUCHTypes::E;
      default: return OUCHTypes::unknownOUCHtype;
   }
}

}
