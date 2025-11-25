// Generated automatically. DO NOT EDIT!
// BIST ITCH Messages Struct Definitions
#pragma once

#include <cstdint>

namespace BIST {

struct alignas(64) ITCHSecondsMessage {
    uint32_t second = 0;            // Unix time (saniye) | offset: 1, len: 4
    char message_type = 'T';        // Mesaj Türü (T = Seconds) | offset: 0, len: 1
    char pad[59];                   // Doldurma (Padding)
};

struct alignas(64) ITCHOrderBookDirectoryMessage {
    uint64_t nominal_value = 0;                   // Nominal value | offset: 105, len: 8
    uint32_t timestamp_ns = 0;                    // Nanoseconds kısmı | offset: 1, len: 4
    uint32_t order_book_id = 0;                   // Order book ID | offset: 5, len: 4
    uint32_t odd_lot_size = 0;                    // Odd lot size | offset: 93, len: 4
    uint32_t round_lot_size = 0;                  // Round lot size | offset: 97, len: 4
    uint32_t block_lot_size = 0;                  // Block lot size | offset: 101, len: 4
    uint32_t underlying_order_book_id = 0;        // Underlying order book ID | offset: 114, len: 4
    int32_t strike_price = 0;                     // Strike price (derivatives) | offset: 118, len: 4
    uint32_t expiration_date = 0;                 // Expiration date YYYYMMDD | offset: 122, len: 4
    uint16_t decimals_in_price = 0;               // Fiyat ondalık basamak sayısı | offset: 89, len: 2
    uint16_t decimals_in_nominal = 0;             // Nominal değer ondalık basamak | offset: 91, len: 2
    uint16_t decimals_in_strike_price = 0;        // Strike price ondalık | offset: 126, len: 2
    uint8_t financial_product = 0;                // Financial product (1=Option, 5=Cash, etc.) | offset: 85, len: 1
    uint8_t number_of_legs = 0;                   // Number of legs (kombinasyon için) | offset: 113, len: 1
    uint8_t put_or_call = 0;                      // Put/Call (1=Call, 2=Put) | offset: 128, len: 1
    uint8_t ranking_type = 0;                     // Ranking type (1=Price Time) | offset: 129, len: 1
    char message_type = 'R';                      // Mesaj Türü (R = Order Book Directory) | offset: 0, len: 1
    char isin[12] = {};                           // ISIN kodu | offset: 73, len: 12
    char pad1[1];                                 // Doldurma (Padding)
    char symbol[32] = {};                         // Security short name | offset: 9, len: 32
    char long_name[32] = {};                      // Long name | offset: 41, len: 32
    char trading_currency[3] = {};                // Trading currency | offset: 86, len: 3
    char pad2[61];                                // Doldurma (Padding)
};

struct alignas(64) ITCHCombinationOrderBookLegMessage {
    uint32_t timestamp_ns = 0;                     // Nanoseconds kısmı | offset: 1, len: 4
    uint32_t combination_order_book_id = 0;        // Combination order book ID | offset: 5, len: 4
    uint32_t leg_order_book_id = 0;                // Leg order book ID | offset: 9, len: 4
    uint32_t leg_ratio = 0;                        // Leg ratio | offset: 14, len: 4
    char message_type = 'M';                       // Mesaj Türü (M = Combination Leg) | offset: 0, len: 1
    char leg_side = '\0';                          // Leg side (B=As Defined, C=Opposite) | offset: 13, len: 1
    char pad[46];                                  // Doldurma (Padding)
};

struct alignas(64) ITCHTickSizeTableEntryMessage {
    uint64_t tick_size = 0;            // Tick size (8 byte!) | offset: 9, len: 8
    uint32_t timestamp_ns = 0;         // Nanoseconds kısmı | offset: 1, len: 4
    uint32_t order_book_id = 0;        // Order book ID | offset: 5, len: 4
    int32_t price_from = 0;            // Price from | offset: 17, len: 4
    int32_t price_to = 0;              // Price to (0=infinity) | offset: 21, len: 4
    char message_type = 'L';           // Mesaj Türü (L = Tick Size) | offset: 0, len: 1
    char pad[39];                      // Doldurma (Padding)
};

struct alignas(64) ITCHShortSellStatusMessage {
    uint32_t timestamp_ns = 0;                 // Nanoseconds kısmı | offset: 1, len: 4
    uint32_t order_book_id = 0;                // Order book ID (Equity only) | offset: 5, len: 4
    uint8_t short_sale_restriction = 0;        // Short sell status (0=No restriction, 1=Not allowed, 2=Uptick rule) | offset: 9, len: 1
    char message_type = 'V';                   // Mesaj Türü (V = Short Sell Status) | offset: 0, len: 1
    char pad[54];                              // Doldurma (Padding)
};

struct alignas(64) ITCHSystemEventMessage {
    uint32_t timestamp_ns = 0;        // Nanoseconds kısmı | offset: 1, len: 4
    char message_type = 'S';          // Mesaj Türü (S = Sistem Olayı) | offset: 0, len: 1
    char event_code = '\0';           // Olay kodu (O=Start, C=End) | offset: 5, len: 1
    char pad[58];                     // Doldurma (Padding)
};

struct alignas(64) ITCHOrderBookStateMessage {
    uint32_t timestamp_ns = 0;         // Nanoseconds kısmı | offset: 1, len: 4
    uint32_t order_book_id = 0;        // Order book ID | offset: 5, len: 4
    char message_type = 'O';           // Mesaj Türü (O = Order Book State) | offset: 0, len: 1
    char state_name[20] = {};          // State name | offset: 9, len: 20
    char pad[35];                      // Doldurma (Padding)
};

struct alignas(64) ITCHAddOrderMessage {
    uint64_t order_id = 0;                  // Order ID (benzersiz değil! order_book_id + side ile birlikte) | offset: 5, len: 8
    uint64_t quantity = 0;                  // Visible quantity | offset: 22, len: 8
    uint64_t ranking_time = 0;              // Ranking timestamp - nanosecond | offset: 37, len: 8
    uint32_t timestamp_ns = 0;              // Nanoseconds kısmı | offset: 1, len: 4
    uint32_t order_book_id = 0;             // Order book ID | offset: 13, len: 4
    uint32_t ranking_seq_number = 0;        // Ranking sequence number | offset: 18, len: 4
    int32_t price = 0;                      // Display price (SIGNED!) | offset: 30, len: 4
    uint16_t order_attributes = 0;          // Order attributes (0=N/A, 8192=Bait) | offset: 34, len: 2
    uint8_t lot_type = 0;                   // Lot type (2=Round Lot) | offset: 36, len: 1
    char message_type = 'A';                // Mesaj Türü (A = Add Order) | offset: 0, len: 1
    char side = '\0';                       // Side (B=Buy, S=Sell) | offset: 17, len: 1
    char pad[19];                           // Doldurma (Padding)
};

struct alignas(64) ITCHOrderExecutedMessage {
    uint64_t order_id = 0;                 // Order ID | offset: 5, len: 8
    uint64_t executed_quantity = 0;        // Executed quantity | offset: 18, len: 8
    uint64_t match_id = 0;                 // Match ID | offset: 26, len: 8
    uint32_t timestamp_ns = 0;             // Nanoseconds kısmı | offset: 1, len: 4
    uint32_t order_book_id = 0;            // Order book ID | offset: 13, len: 4
    uint32_t combo_group_id = 0;           // Combo group ID | offset: 34, len: 4
    char message_type = 'E';               // Mesaj Türü (E = Order Executed) | offset: 0, len: 1
    char side = '\0';                      // Side (B=Buy, S=Sell) | offset: 17, len: 1
    char reserved1[7] = {};                // Reserved | offset: 38, len: 7
    char reserved2[7] = {};                // Reserved | offset: 45, len: 7
    char pad[12];                          // Doldurma (Padding)
};

struct alignas(64) ITCHOrderExecutedWithPriceMessage {
    uint64_t order_id = 0;                 // Order ID | offset: 5, len: 8
    uint64_t executed_quantity = 0;        // Executed quantity | offset: 18, len: 8
    uint64_t match_id = 0;                 // Match ID | offset: 26, len: 8
    uint32_t timestamp_ns = 0;             // Nanoseconds kısmı | offset: 1, len: 4
    uint32_t order_book_id = 0;            // Order book ID | offset: 13, len: 4
    uint32_t combo_group_id = 0;           // Combo group ID | offset: 34, len: 4
    int32_t trade_price = 0;               // Trade price (SIGNED!) | offset: 52, len: 4
    char message_type = 'C';               // Mesaj Türü (C = Order Executed with Price) | offset: 0, len: 1
    char side = '\0';                      // Side (B=Buy, S=Sell) | offset: 17, len: 1
    char reserved1[7] = {};                // Reserved | offset: 38, len: 7
    char reserved2[7] = {};                // Reserved | offset: 45, len: 7
    char occurred_at_cross = '\0';         // Occurred at cross (Y/N) | offset: 56, len: 1
    char printable = '\0';                 // Printable (Y/N) | offset: 57, len: 1
    char pad[6];                           // Doldurma (Padding)
};

struct alignas(64) ITCHOrderDeleteMessage {
    uint64_t order_id = 0;             // Order ID | offset: 5, len: 8
    uint32_t timestamp_ns = 0;         // Nanoseconds kısmı | offset: 1, len: 4
    uint32_t order_book_id = 0;        // Order book ID | offset: 13, len: 4
    char message_type = 'D';           // Mesaj Türü (D = Order Delete) | offset: 0, len: 1
    char side = '\0';                  // Side (B=Buy, S=Sell) | offset: 17, len: 1
    char pad[46];                      // Doldurma (Padding)
};

struct alignas(64) ITCHOrderBookFlushMessage {
    uint32_t timestamp_ns = 0;         // Nanoseconds kısmı | offset: 1, len: 4
    uint32_t order_book_id = 0;        // Order book ID | offset: 5, len: 4
    char message_type = 'Y';           // Mesaj Türü (Y = Order Book Flush) | offset: 0, len: 1
    char pad[55];                      // Doldurma (Padding)
};

struct alignas(64) ITCHTradeMessage {
    uint64_t match_id = 0;                // Match ID | offset: 5, len: 8
    uint64_t quantity = 0;                // Quantity | offset: 18, len: 8
    uint32_t timestamp_ns = 0;            // Nanoseconds kısmı | offset: 1, len: 4
    uint32_t combo_group_id = 0;          // Combo group ID | offset: 13, len: 4
    uint32_t order_book_id = 0;           // Order book ID | offset: 26, len: 4
    int32_t trade_price = 0;              // Trade price | offset: 30, len: 4
    char message_type = 'P';              // Mesaj Türü (P = Trade) | offset: 0, len: 1
    char side = '\0';                     // Side (B/S/Space) | offset: 17, len: 1
    char reserved1[7] = {};               // Reserved | offset: 34, len: 7
    char reserved2[7] = {};               // Reserved | offset: 41, len: 7
    char printable = '\0';                // Printable (Y/N) | offset: 48, len: 1
    char occurred_at_cross = '\0';        // Occurred at cross (Y/N) | offset: 49, len: 1
    char pad[14];                         // Doldurma (Padding)
};

struct alignas(64) ITCHEquilibriumPriceUpdateMessage {
    uint64_t available_bid_quantity = 0;        // Bid quantity at equilibrium | offset: 9, len: 8
    uint64_t available_ask_quantity = 0;        // Ask quantity at equilibrium | offset: 17, len: 8
    uint64_t best_bid_quantity = 0;             // Best bid quantity | offset: 37, len: 8
    uint64_t best_ask_quantity = 0;             // Best ask quantity | offset: 45, len: 8
    uint32_t timestamp_ns = 0;                  // Nanoseconds kısmı | offset: 1, len: 4
    uint32_t order_book_id = 0;                 // Order book ID | offset: 5, len: 4
    int32_t equilibrium_price = 0;              // Equilibrium price | offset: 25, len: 4
    int32_t best_bid_price = 0;                 // Best bid price | offset: 29, len: 4
    int32_t best_ask_price = 0;                 // Best ask price | offset: 33, len: 4
    char message_type = 'Z';                    // Mesaj Türü (Z = Equilibrium Price) | offset: 0, len: 1
    char pad[11];                               // Doldurma (Padding)
};

enum class ITCHTypes : uint8_t {   T = 0,  R = 1,  M = 2,  L = 3,  V = 4,  S = 5,  O = 6,  A = 7,  E = 8,  C = 9,  D = 10,  Y = 11,  P = 12,  Z = 13,  unknownITCHtype = 99 };

inline constexpr ITCHTypes itchMessageIndex(char type) {

   switch(type) {
      case 'T': return ITCHTypes::T;
      case 'R': return ITCHTypes::R;
      case 'M': return ITCHTypes::M;
      case 'L': return ITCHTypes::L;
      case 'V': return ITCHTypes::V;
      case 'S': return ITCHTypes::S;
      case 'O': return ITCHTypes::O;
      case 'A': return ITCHTypes::A;
      case 'E': return ITCHTypes::E;
      case 'C': return ITCHTypes::C;
      case 'D': return ITCHTypes::D;
      case 'Y': return ITCHTypes::Y;
      case 'P': return ITCHTypes::P;
      case 'Z': return ITCHTypes::Z;
      default: return ITCHTypes::unknownITCHtype;
   }
}

}
