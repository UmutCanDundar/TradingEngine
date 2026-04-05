// FIX protocol message representations used by the market data and order
// management pipeline.
//
// These structures are populated directly from the TCP stream by the FIX
// parser and passed by pointer to downstream components (e.g. OrderManager).
//
// Cache-line alignment optimization is not implemented for these structures 
// as they are written by a single thread and treated as read-only afterwards.

#pragma once

#include <cstdint>
#include <cstddef>

#include <boost/lockfree/spsc_queue.hpp>

enum class FIXTypes : uint8_t
{
    Logon = 'A',
    Logout = '5',
    Heartbeat = '0',
    TestRequest = '1',
    ResendRequest = '2',
    Reject = '3',
    SequenceReset = '4',
    NewOrderSingle = 'D',
    OrderCancelRequest = 'F',
    OrderCancelReplaceRequest = 'G',
    ExecutionReport = '8',
    OrderCancelReject = '9',
};

enum class LogoutStatus : uint8_t
{
    Password_Error = 3,
    Logged_out = 4,
    Invalid_userinfo = 5,
    Account_locked = 6,
    Password_expired = 8,
    Invalid_id = 9,
    Invalid_bodylength = 100, // crtical error-session suspended --- requires manual intervention
    Low_interval = 101,
    Unknown = 255

};

enum class SessionRejectReason : uint8_t
{
    Invalid_tag_number = 0,
    Required_tag_missing = 1,
    Tag_not_defined_for_this_type = 2,
    Undefined_tag = 3,
    Tag_specified_without_a_value = 4,
    Out_of_range_value = 5,
    Incorrect_data_format_for_value = 6,
    CompID_problem = 9,
    Sendingtime_accuracy_problem = 10,
    Invalid_type = 11,
    Repeating_group_fields = 15,
    Incorrect_numInGroup = 16,
    Other = 99,
    Unknown = 255
};

enum class CxlRejectReason : uint8_t
{
    Invalid_tag_number = 0,
    Required_tag_missing = 1,
    Tag_not_defined_for_this_type = 2,
    Incorrect_data_format_for_value = 6, // crtical error-session suspended --- requires manual intervention
    Unknown = 255
};

struct FIXMessage // ~216B
{
    static constexpr size_t ID_SIZE = 32;
    static constexpr size_t SYMBOL_SIZE = 32; 

    int64_t price = 0; // FIX Tag 44
    int64_t last_price = 0; // FIX Tag 31

    uint32_t quantity = 0;      // FIX Tag 38
    uint32_t leaves_qty = 0;    // FIX Tag 151
    uint32_t last_qty = 0;      // FIX Tag 14
    uint32_t filled_qty = 0;    // FIX Tag 32 cumulative
    uint32_t transact_time = 0; // FIX Tag 60 seconds
    uint32_t instrument_id = 0; // FIX Tag 48
    uint32_t seqnum = 0;        // FIX Tag 34

    uint8_t msg_type = 0;      // FIX Tag 35
    uint8_t side = 0;          // FIX Tag 54
    uint8_t ord_status = 0;    // FIX Tag 39
    uint8_t exec_type = 0;     // FIX Tag 150
    uint8_t ord_type = 0;      // FIX Tag 40
    uint8_t time_in_force = 0; // FIX Tag 59
    uint8_t cxl_rej_response_to = 0; // FIX Tag 434
    uint8_t cxl_rej_reason = 255;    // FIX Tag 102

    char symbol[SYMBOL_SIZE] = {}; // FIX Tag 55
    char order_id[ID_SIZE] = {};   // FIX Tag 37
    char cl_ord_id[ID_SIZE] = {};  // FIX Tag 11
    char exec_id[ID_SIZE] = {};    // FIX Tag 17
    char orig_cl_ord_id[ID_SIZE] = {}; // FIX Tag 41

};

struct FIXSessionMessage // ~80B
{
    static constexpr size_t TEXT_SIZE = 32;

    uint32_t seqnum = 0; // FIX Tag 34
    uint32_t interval = 0; // FIX Tag 108
    uint32_t ses_status = 255; // FIX Tag 1409
    uint32_t new_seqnum = 0;   // FIX Tag 36
    uint32_t begin_seqnum = 0; // FIX Tag 7
    uint32_t end_seqnum = 0;   // FIX Tag 16
    uint32_t ref_seqnum = 0;   // FIX Tag 45
    uint32_t test_req_id = 0;   // FIX Tag 112
    uint32_t sending_time = 0;     // FIX Tag 52
    uint32_t org_sending_time = 0; // FIX Tag 122

    uint8_t gap_fill_flag = 0;   // FIX Tag 123
    uint8_t reject_reason = 255; // FIX Tag 373
    uint8_t msg_type = 0;        // FIX Tag 35
    uint8_t possDubFlag = 0;     // FIX Tag 43
    uint8_t reset_seqnum_flag = 'N'; // FIX Tag 141

    char text[32] = {}; // FIX Tag 58
};

inline constexpr size_t FIX_QUEUE_CAPACITY = 1024;

using spscFIXInSessionQueue_t = boost::lockfree::spsc_queue<FIXSessionMessage *, boost::lockfree::capacity<FIX_QUEUE_CAPACITY>>;
using spscFIXOutSessionQueue_t = boost::lockfree::spsc_queue<FIXSessionMessage *, boost::lockfree::capacity<FIX_QUEUE_CAPACITY>>;
