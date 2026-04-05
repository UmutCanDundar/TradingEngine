// ==================================================================================================
// Builder_FIX
//
// PURPOSE:
// - High-performance FIX message builder responsible for constructing
//   administrative and application-level FIX messages with minimal latency.
//
// THREAD SAFETY:
// - Not thread-safe.
// - Intended to be used per-session or per-thread.
// - Internal temporary buffers (temp_bufs) assume single-threaded access.

// PERFORMANCE & DESIGN NOTES:
// - Uses compile-time dispatch via templates and std::integral_constant
//   instead of runtime switches.
// - Hot-path functions are selectively inlined to reduce call overhead
//   without excessive instruction-cache pressure.
// - Heavy logic (header building, checksum, parsing) is intentionally
//   placed in .cpp to control code size.
// - Avoids dynamic allocation entirely.
// - SIMD (AVX2) is used for FIX tag scanning and parsing.
// - FIX fields are appended using pointer arithmetic and memcpy only.

// DEVELOPER NOTES (PRE-PROFILING):
// - Instruction cache pressure should be evaluated under real message rates.
// - transact_time() currently formats timestamps per call; consider
//   caching or preformatting if profiling indicates pressure.
// - This class is currently owned by Builder_Dispatch, and its memory location depends on that owner.
//   If this class is seperated in the future:
//    * Raw object size is ~33 KB, however allocation decisions for this class are NOT based on size.
//    * According to the current architecture, ownership is expected to remain at a higher-level
//      controller (e.g. TradingEngine, heap-allocated) and accessed by a dedicated worker thread,
//      making stack allocation incompatible with the ownership and lifetime model.
//    * Even if the ownership model were changed (e.g. made thread-local or per-session), this would
//      still be a long-lived, stateful core component whose lifetime is tied to the owning context
//      rather than a single stack frame.
//    * Additionally, this component is expected to evolve and potentially grow in size as protocol
//      handling and internal state expand over time.
//      Therefore, such core components MUST be heap-allocated regardless of their raw size.
// ===================================================================================================

#pragma once

#include "FIXMessage.h"
#include "Order.h"
#include "SessionManager.h"

#include <array>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string_view>
#include <type_traits>

class Builder_FIX 
{

private:
    static constexpr char SOH = '\x01';
    static constexpr size_t MAX_BODY_SIZE = 256;

    static constexpr size_t TEMP_BUFFER_SIZE = 64;
    std::array<Buffer_FIX, TEMP_BUFFER_SIZE> temp_bufs;
    size_t temp_index = 0;

    static constexpr auto msg_types = []
    {
        std::array<const char*, 128> arr = {};
        arr[static_cast<size_t>(FIXTypes::Logon)] = "A";
        arr[static_cast<size_t>(FIXTypes::Logout)] = "5";
        arr[static_cast<size_t>(FIXTypes::Heartbeat)] = "0";
        arr[static_cast<size_t>(FIXTypes::TestRequest)] = "1";
        arr[static_cast<size_t>(FIXTypes::ResendRequest)] = "2";
        arr[static_cast<size_t>(FIXTypes::SequenceReset)] = "4";
        arr[static_cast<size_t>(FIXTypes::NewOrderSingle)] = "D";
        arr[static_cast<size_t>(FIXTypes::OrderCancelRequest)] = "F";
        arr[static_cast<size_t>(FIXTypes::OrderCancelReplaceRequest)] = "G";

        return arr;
    }();

    static constexpr auto timeinforce = []
    {
        std::array<const char* , 128> arr = {};
        arr[static_cast<size_t>(TimeInForce::DAY)] = "0";
        arr[static_cast<size_t>(TimeInForce::GTC)] = "1";
        arr[static_cast<size_t>(TimeInForce::IOC)] = "3";
        arr[static_cast<size_t>(TimeInForce::FOK)] = "4";
        arr[static_cast<size_t>(TimeInForce::GTD)] = "6";
        arr[static_cast<size_t>(TimeInForce::AT_CROSSING)] = "9";
        arr[static_cast<size_t>(TimeInForce::GTS)] = "S";

        return arr;
    }();

    SessionManager& sess_mngr_;
    
public:
    Builder_FIX(SessionManager& sess_mngr) noexcept;
    Buffer_FIX *build_resend(const Buffer_FIX *org_buf, uint8_t session_index) noexcept;

    template <FIXTypes T, typename... Args>
    Buffer_FIX* build(uint8_t session_index, Args&&... args) noexcept
    {
        auto& seq_fix = sess_mngr_.getSessionState(session_index)->fix;
        auto& auth_fix = sess_mngr_.getSessionAuth(sess_mngr_.getSessionContext(session_index)->tcp_index)->fix;

        Buffer_FIX* buffer = seq_fix.get_buffer(seq_fix.get_next_seq());
        
        char hotdata_temp[MAX_BODY_SIZE];
        char *hotdata_end_ptr = Builder_FIX::buildBody<T>(hotdata_temp, session_index, std::forward<Args>(args)...);
        size_t hotdata_len = static_cast<size_t>(hotdata_end_ptr - hotdata_temp);
        
        char* buf_end_ptr = buildHeader(buffer->data, hotdata_len, static_cast<size_t>(T), auth_fix, seq_fix);
        std::memcpy(buf_end_ptr, hotdata_temp, hotdata_len);
        buffer->len = static_cast<size_t>(buf_end_ptr + hotdata_len - buffer->data);
        
        buffer->hotdata_start = buf_end_ptr;
        buffer->hotdata_len = hotdata_len;

        buf_end_ptr = finalizeChecksum(buffer->data, buffer->len);
        buffer->len += 7;
        
        seq_fix.increase_next_seq();
       
        return buffer;
    }

    template <FIXTypes T, typename... Args>
    inline char* buildBody(char* buf, uint8_t session_index, Args&&... args) noexcept {
        return buildBody_impl(std::integral_constant<FIXTypes, T>{},
                              buf, session_index, std::forward<Args>(args)...);
    }

private:

    // ------------------- HEADER ---------------------
     char* buildHeader(char* buf, size_t body_len, const size_t msg_index, const VenueUserInfo_FIX& auth_fix, Sequence_FIX& seq_fix) noexcept;
     char* buildHeader(const Buffer_FIX *org_buf, char *temp, const VenueUserInfo_FIX &auth_fix) noexcept;


     // ------------------- BODY -------------------------
     // ADMINISTRATIVE MESSAGES
    char* buildLogon(char* buf, uint8_t session_index, const bool reset = false) noexcept; 
    inline char* buildLogout(char* buf) noexcept { return buf; }
    inline char* buildHeartbeat(char* buf) noexcept { return buf; }
    char* buildTestRequest(char* buf) noexcept;
    char* buildSequenceReset(char* buf, const uint32_t new_seq) noexcept;
    char* buildResendRequest(char* buf, const uint32_t begin_seq, const uint32_t end_seq) noexcept;


    // --- APPLICATION MESSAGES ---
    char* buildNewOrderSingle(const Order &order, char* buf, uint8_t session_index) noexcept;
    char* buildCancelRequest(const Order& order, char* buf) noexcept; 
    char* buildCancelReplaceRequest(const Order &order, char* buf, uint8_t session_index) noexcept;


    // --- TEMPLATE IMPLEMENTATIONS FOR THE BODY BUILDERS ---
    inline char* buildBody_impl(std::integral_constant<FIXTypes, FIXTypes::Logon>, char* buf, uint8_t session_index, const bool reset) noexcept
    {
        return Builder_FIX::buildLogon(buf, session_index, reset);
    }
    inline char *buildBody_impl(std::integral_constant<FIXTypes, FIXTypes::SequenceReset>, char *buf, uint8_t /*session_index*/, const uint32_t new_seq) noexcept
    {
        return Builder_FIX::buildSequenceReset(buf, new_seq);
    }
    inline char *buildBody_impl(std::integral_constant<FIXTypes, FIXTypes::ResendRequest>, char *buf, uint8_t /*session_index*/, const uint32_t begin_seq, const uint32_t end_seq) noexcept
    {
        return Builder_FIX::buildResendRequest(buf, begin_seq, end_seq);
    }
    inline char *buildBody_impl(std::integral_constant<FIXTypes, FIXTypes::Logout>, char *buf, uint8_t /*session_index*/) noexcept
    {
        return Builder_FIX::buildLogout(buf);
    }
    inline char *buildBody_impl(std::integral_constant<FIXTypes, FIXTypes::Heartbeat>, char *buf, uint8_t /*session_index*/) noexcept
    {
        return Builder_FIX::buildHeartbeat(buf);
    }
    inline char *buildBody_impl(std::integral_constant<FIXTypes, FIXTypes::TestRequest>, char *buf, uint8_t /*session_index*/) noexcept
    {
        return Builder_FIX::buildTestRequest(buf);
    }
    inline char *buildBody_impl(std::integral_constant<FIXTypes, FIXTypes::NewOrderSingle>, char *buf, uint8_t session_index, const Order &order) noexcept
    {
        return Builder_FIX::buildNewOrderSingle(order, buf, session_index);
    }
    inline char *buildBody_impl(std::integral_constant<FIXTypes, FIXTypes::OrderCancelRequest>, char *buf, uint8_t /*session_index*/, const Order &order) noexcept
    {
        return Builder_FIX::buildCancelRequest(order, buf);
    }
    inline char *buildBody_impl(std::integral_constant<FIXTypes, FIXTypes::OrderCancelReplaceRequest>, char *buf, uint8_t session_index, const Order &order) noexcept
    {
        return Builder_FIX::buildCancelReplaceRequest(order, buf, session_index);
    }


    // -------------------  TRAILER ----------------------
    char* finalizeChecksum(char *buf, size_t len) noexcept; 
 

    // -------------------  UTIL FUNC --------------------
    static std::pair<const char *, const size_t> transact_time() noexcept;
    static char findType(const char *data) noexcept;
    static std::pair<const char *, size_t> findValue(const char *data, size_t len, std::string_view wantTag) noexcept;
    static char* placeIntValue(uint32_t num, char* buf) noexcept;
    
    static inline int strnum_to_int(const std::string_view strnum) noexcept
    {
        int result = 0;
        for (const unsigned char c : strnum)
            result = result * 10 + (c - '0');   

        return result;
    }

    static inline uint8_t digit(const auto num) noexcept
    {
        return (num >= 100000) ? 6 : (num >= 10000) ? 5
                                 : (num >= 1000)    ? 4
                                 : (num >= 100)     ? 3
                                 : (num >= 10)      ? 2
                                                    : 1;
    }

    static inline char* addTag(const char *tag, const size_t taglen, const char *val, const size_t vallen, char *buf) noexcept
    {
        std::memcpy(buf, tag, taglen);
        buf += taglen;
        std::memcpy(buf, val, vallen);
        buf += vallen;
        *buf++ = SOH;

        return buf;
    }
    static inline char* addTag(const char *tag, const size_t taglen, uint32_t val, char *buf) noexcept
    {
        std::memcpy(buf, tag, taglen);
        buf += taglen;
        buf = placeIntValue(val, buf);
        *buf++ = SOH;

        return buf;
    }
   
};

