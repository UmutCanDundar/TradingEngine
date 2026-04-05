// Generated automatically. DO NOT EDIT!
// NASDAQ OUCH Messages Struct Definitions
#pragma once

#include <cstdint>

namespace NASDAQ {

/* namespace IN {

struct OUCHEnterOrderMessage {
    uint64_t price = 0;                              // Price with 4 implied decimals | offset: 18, len: 8
    uint32_t user_ref_num = 0;                       // Day-unique, strictly increasing ID | offset: 1, len: 4
    uint32_t quantity = 0;                           // Total shares (must be >0 and <1,000,000) | offset: 6, len: 4
    uint16_t appendage_length = 0;                   // Length of optional appendage | offset: 45, len: 2
    char message_type = 'O';                         // Enter Order Message | offset: 0, len: 1
    char side = '\0';                                // B=Buy, S=Sell, T=SellShort, E=SellShortExempt | offset: 5, len: 1
    char symbol[8] = {};                             // Stock symbol | offset: 10, len: 8
    char time_in_force = '0';                        // 0=Day, 3=IOC, 5=GTX, 6=GTT, E=AfterHours | offset: 26, len: 1
    char display = 'Y';                              // Y=Visible, N=Hidden, A=Attributable | offset: 27, len: 1
    char capacity = 'A';                             // A=Agency, P=Principal, R=Riskless, O=Other | offset: 28, len: 1
    char intermarket_sweep_eligibility = 'N';        // Y=Eligible, N=NotEligible | offset: 29, len: 1
    char cross_type = 'N';                           // N=Continuous, O=Opening, C=Closing, H=Halt/IPO, S=Supplemental, R=Retail, E=ExtendedLife, A=AfterHoursClose | offset: 30, len: 1
    char cl_ord_id[14] = {};                         // Customer order identifier | offset: 31, len: 14
    //char optional_appendage[0] = {};               // Optional fields (Firm, MinQty, CustomerType, MaxFloor, PriceType, PegOffset, etc.) | offset: 47, len: variable (Will be appended to the buffer in the parser/builder when necessary, not actually stored in the struct)
};

struct OUCHReplaceOrderMessage {
    uint64_t price = 0;                              // Replacement price with 4 implied decimals | offset: 13, len: 8
    uint32_t orig_user_ref_num = 0;                  // Original order UserRefNum | offset: 1, len: 4
    uint32_t user_ref_num = 0;                       // New replacement UserRefNum (must be unique) | offset: 5, len: 4
    uint32_t quantity = 0;                           // Total shares liable (inclusive of previous executions) | offset: 9, len: 4
    uint16_t appendage_length = 0;                   // Length of optional appendage | offset: 38, len: 2
    char message_type = 'u';                         // Replace Order Message | offset: 0, len: 1
    char time_in_force = '0';                        // 0=Day, 3=IOC, 5=GTX, 6=GTT | offset: 21, len: 1
    char display = 'Y';                              // Y=Visible, N=Hidden, A=Attributable | offset: 22, len: 1
    char intermarket_sweep_eligibility = 'N';        // Y=Eligible, N=NotEligible | offset: 23, len: 1
    char cl_ord_id[14] = {};                         // Customer order identifier for replacement | offset: 24, len: 14
    //char optional_appendage[0] = {};               // Optional fields (MinQty, MaxFloor, PriceType, PostOnly, ExpireTime, TradeNow, HandleInst, etc.) | offset: 40, len: variable (Will be appended to the buffer in the parser/builder when necessary, not actually stored in the struct)
};

struct OUCHCancelOrderMessage {
    uint32_t user_ref_num = 0;                // Order UserRefNum to cancel | offset: 1, len: 4
    uint32_t quantity = 0;                    // New intended order size (0=cancel all remaining) | offset: 5, len: 4
    uint16_t appendage_length = 0;            // Length of optional appendage | offset: 9, len: 2
    char message_type = 'X';                  // Cancel Order Message | offset: 0, len: 1
    //char optional_appendage[0] = {};        // Optional fields (UserRefIdx) | offset: 11, len: variable (Will be appended to the buffer in the parser/builder when necessary, not actually stored in the struct)
};

struct OUCHModifyOrderMessage {
    uint32_t user_ref_num = 0;                // Order UserRefNum to modify | offset: 1, len: 4
    uint32_t quantity = 0;                    // New intended order size | offset: 6, len: 4
    uint16_t appendage_length = 0;            // Length of optional appendage | offset: 10, len: 2
    char message_type = 'm';                  // Modify Order Message | offset: 0, len: 1
    char side = '\0';                         // New side: B=Buy, S=Sell, T=SellShort, E=SellShortExempt (only S<->E, S<->T, E<->T allowed) | offset: 5, len: 1
    //char optional_appendage[0] = {};        // Optional fields (UserRefIdx, SharesLocated, LocateBroker) | offset: 12, len: variable (Will be appended to the buffer in the parser/builder when necessary, not actually stored in the struct)
};

struct OUCHAccountQueryRequest {
    uint16_t appendage_length = 0;            // Length of optional appendage | offset: 1, len: 2
    char message_type = 'q';                  // Account Query Request | offset: 0, len: 1
    //char optional_appendage[0] = {};        // Optional fields (UserRefIdx) | offset: 3, len: variable (Will be appended to the buffer in the parser/builder when necessary, not actually stored in the struct)
};

} */ 
namespace OUT
{

struct OUCHSystemEventMessage {
    uint64_t timestamp = 0;         // Nanoseconds since midnight | offset: 1, len: 8
    char message_type = 'S';        // System Event Message | offset: 0, len: 1
    char event_code = '\0';         // S=StartOfDay, E=EndOfDay | offset: 9, len: 1
};

struct OUCHOrderAcceptedMessage {
    uint64_t timestamp = 0;                           // Nanoseconds since midnight | offset: 1, len: 8
    uint64_t price = 0;                               // Accepted price (may differ from entered) | offset: 26, len: 8
    uint64_t order_reference_number = 0;              // Day-unique order reference assigned by NASDAQ | offset: 36, len: 8
    uint32_t user_ref_num = 0;                        // Echo of UserRefNum | offset: 9, len: 4
    uint32_t quantity = 0;                            // Total shares accepted | offset: 14, len: 4
    uint16_t appendage_length = 0;                    // Length of optional appendage | offset: 62, len: 2
    char message_type = 'A';                          // Order Accepted Message | offset: 0, len: 1
    char side = '\0';                                 // B/S/T/E | offset: 13, len: 1
    char symbol[8] = {};                              // Stock symbol | offset: 18, len: 8
    char time_in_force = '0';                         // Accepted TIF | offset: 34, len: 1
    char display = '\0';                              // Y=Visible, N=Hidden, A=Attributable, Z=Conformant | offset: 35, len: 1
    char capacity = '\0';                             // A/P/R/O | offset: 44, len: 1
    char intermarket_sweep_eligibility = '\0';        // Y/N | offset: 45, len: 1
    char cross_type = '\0';                           // N/O/C/H/S/R/E/A | offset: 46, len: 1
    char order_state = '\0';                          // L=OrderLive, D=OrderDead | offset: 47, len: 1
    char cl_ord_id[14] = {};                          // Customer order identifier | offset: 48, len: 14
    //char optional_appendage[0] = {};                // Optional fields (Firm, MinQty, CustomerType, MaxFloor, PriceType, etc.) | offset: 64, len: variable (Will be appended to the buffer in the parser/builder when necessary, not actually stored in the struct)
};

struct OUCHOrderReplacedMessage {
    uint64_t timestamp = 0;                           // Nanoseconds since midnight | offset: 1, len: 8
    uint64_t price = 0;                               // Accepted replacement price | offset: 30, len: 8
    uint64_t order_reference_number = 0;              // Day-unique order reference | offset: 40, len: 8
    uint32_t orig_user_ref_num = 0;                   // Original UserRefNum | offset: 9, len: 4
    uint32_t user_ref_num = 0;                        // Replacement UserRefNum | offset: 13, len: 4
    uint32_t quantity = 0;                            // Total shares outstanding | offset: 18, len: 4
    uint16_t appendage_length = 0;                    // Length of optional appendage | offset: 66, len: 2
    char message_type = 'U';                          // Order Replaced Message | offset: 0, len: 1
    char side = '\0';                                 // B/S/T/E | offset: 17, len: 1
    char symbol[8] = {};                              // Stock symbol | offset: 22, len: 8
    char time_in_force = '0';                         // Accepted TIF | offset: 38, len: 1
    char order_state = '\0';                          // L=OrderLive, D=OrderDead | offset: 51, len: 1
    char cl_ord_id[14] = {};                          // Customer order identifier | offset: 52, len: 14
    char display = '\0';                              // Y=Visible, N=Hidden, A=Attributable, Z=Conformant | offset: 39, len: 1
    char capacity = '\0';                             // A/P/R/O | offset: 48, len: 1
    char intermarket_sweep_eligibility = '\0';        // Y/N | offset: 49, len: 1
    char cross_type = '\0';                           // N/O/C/H/S/R/E/A | offset: 50, len: 1
    //char optional_appendage[0] = {};                // Optional fields (Firm, MinQty, MaxFloor, PriceType, etc.) | offset: 68, len: variable (Will be appended to the buffer in the parser/builder when necessary, not actually stored in the struct)
};

struct OUCHOrderCancelledMessage {
    uint64_t timestamp = 0;                   // Nanoseconds since midnight | offset: 1, len: 8
    uint32_t user_ref_num = 0;                // Order UserRefNum | offset: 9, len: 4
    uint32_t quantity = 0;                    // Incremental shares canceled | offset: 13, len: 4
    uint16_t appendage_length = 0;            // Length of optional appendage | offset: 18, len: 2
    char message_type = 'C';                  // Order Canceled Message | offset: 0, len: 1
    char reason = '\0';                       // Cancel reason (see Appendix B) | offset: 17, len: 1
    //char optional_appendage[0] = {};        // Optional fields (UserRefIdx) | offset: 20, len: variable (Will be appended to the buffer in the parser/builder when necessary, not actually stored in the struct)
};

struct OUCHOrderExecutedMessage {
    uint64_t timestamp = 0;                   // Nanoseconds since midnight | offset: 1, len: 8
    uint64_t price = 0;                       // Execution price | offset: 17, len: 8
    uint64_t match_number = 0;                // Exchange-assigned match ID | offset: 26, len: 8
    uint32_t user_ref_num = 0;                // Order UserRefNum | offset: 9, len: 4
    uint32_t quantity = 0;                    // Incremental shares executed | offset: 13, len: 4
    uint16_t appendage_length = 0;            // Length of optional appendage | offset: 34, len: 2
    char message_type = 'E';                  // Order Executed Message | offset: 0, len: 1
    char liquidity_flag = '\0';               // Liquidity flag (see Appendix D) | offset: 25, len: 1
    //char optional_appendage[0] = {};        // Optional fields (UserRefIdx) | offset: 36, len: variable (Will be appended to the buffer in the parser/builder when necessary, not actually stored in the struct)
};

struct OUCHOrderRejectedMessage {
    uint64_t timestamp = 0;                   // Nanoseconds since midnight | offset: 1, len: 8
    uint32_t user_ref_num = 0;                // Rejected UserRefNum (cannot be reused) | offset: 9, len: 4
    uint16_t reason = 0;                      // Reject reason code (see Appendix C) | offset: 13, len: 2
    uint16_t appendage_length = 0;            // Length of optional appendage | offset: 29, len: 2
    char message_type = 'J';                  // Order Rejected Message | offset: 0, len: 1
    char cl_ord_id[14] = {};                  // Customer order identifier | offset: 15, len: 14
    //char optional_appendage[0] = {};        // Optional fields (UserRefIdx) | offset: 31, len: variable (Will be appended to the buffer in the parser/builder when necessary, not actually stored in the struct)
};

struct OUCHBrokenTradeMessage {
    uint64_t timestamp = 0;                   // Nanoseconds since midnight | offset: 1, len: 8
    uint64_t match_number = 0;                // Match number from original execution | offset: 13, len: 8
    uint32_t user_ref_num = 0;                // Order UserRefNum | offset: 9, len: 4
    uint16_t appendage_length = 0;            // Length of optional appendage | offset: 36, len: 2
    char message_type = 'B';                  // Broken Trade Message | offset: 0, len: 1
    char reason = '\0';                       // E=Erroneous, C=Consent, S=Supervisory, X=External | offset: 21, len: 1
    char cl_ord_id[14] = {};                  // Customer order identifier | offset: 22, len: 14
    //char optional_appendage[0] = {};        // Optional fields (UserRefIdx) | offset: 38, len: variable (Will be appended to the buffer in the parser/builder when necessary, not actually stored in the struct)
};

struct OUCHOrderModifiedMessage {
    uint64_t timestamp = 0;                   // Nanoseconds since midnight | offset: 1, len: 8
    uint32_t user_ref_num = 0;                // Modified order UserRefNum | offset: 9, len: 4
    uint32_t quantity = 0;                    // Total shares outstanding | offset: 14, len: 4
    uint16_t appendage_length = 0;            // Length of optional appendage | offset: 18, len: 2
    char message_type = 'M';                  // Order Modified Message | offset: 0, len: 1
    char side = '\0';                         // New side (B/S/T/E) | offset: 13, len: 1
    //char optional_appendage[0] = {};        // Optional fields (UserRefIdx, SharesLocated, LocateBroker) | offset: 20, len: variable (Will be appended to the buffer in the parser/builder when necessary, not actually stored in the struct)
};

struct OUCHCancelPendingMessage {
    uint64_t timestamp = 0;                   // Nanoseconds since midnight | offset: 1, len: 8
    uint32_t user_ref_num = 0;                // Order UserRefNum | offset: 9, len: 4
    uint16_t appendage_length = 0;            // Length of optional appendage | offset: 13, len: 2
    char message_type = 'P';                  // Cancel Pending Message | offset: 0, len: 1
    //char optional_appendage[0] = {};        // Optional fields (UserRefIdx) | offset: 15, len: variable (Will be appended to the buffer in the parser/builder when necessary, not actually stored in the struct)
};

struct OUCHCancelRejectMessage {
    uint64_t timestamp = 0;                   // Nanoseconds since midnight | offset: 1, len: 8
    uint32_t user_ref_num = 0;                // Order UserRefNum | offset: 9, len: 4
    uint16_t appendage_length = 0;            // Length of optional appendage | offset: 13, len: 2
    char message_type = 'I';                  // Cancel Reject Message | offset: 0, len: 1
    //char optional_appendage[0] = {};        // Optional fields (UserRefIdx) | offset: 15, len: variable (Will be appended to the buffer in the parser/builder when necessary, not actually stored in the struct)
};

struct OUCHAccountQueryResponse {
    uint64_t timestamp = 0;                   // Nanoseconds since midnight | offset: 1, len: 8
    uint32_t next_user_ref_num = 0;           // Next available UserRefNum | offset: 9, len: 4
    uint16_t appendage_length = 0;            // Length of optional appendage | offset: 13, len: 2
    char message_type = 'Q';                  // Account Query Response | offset: 0, len: 1
    //char optional_appendage[0] = {};        // Optional fields (UserRefIdx) | offset: 15, len: variable (Will be appended to the buffer in the parser/builder when necessary, not actually stored in the struct)
};

}

enum class OUCHTypes : uint8_t {   S = 0,  A = 1,  U = 2,  C = 3,  E = 4,  J = 5,  B = 6,  M = 7,  P = 8,  I = 9,  Q = 10,  unknownOUCHtype = 99 };

inline constexpr OUCHTypes ouchMessageIndex(char type) {

   switch(type) {
      case 'S': return OUCHTypes::S;
      case 'A': return OUCHTypes::A;
      case 'U': return OUCHTypes::U;
      case 'C': return OUCHTypes::C;
      case 'E': return OUCHTypes::E;
      case 'J': return OUCHTypes::J;
      case 'B': return OUCHTypes::B;
      case 'M': return OUCHTypes::M;
      case 'P': return OUCHTypes::P;
      case 'I': return OUCHTypes::I;
      case 'Q': return OUCHTypes::Q;
      default: return OUCHTypes::unknownOUCHtype;
   }
}

}
