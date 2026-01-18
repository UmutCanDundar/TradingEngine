#pragma once

#include "common.h"
#include "Order.h"
#include "SessionManager.h"
#include "Parser_FIX.h"

#include <immintrin.h>
#include <cassert>
#include <chrono>
#include <array>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <atomic>

using spscFIXSessionQueue_t = boost::lockfree::spsc_queue<FIXSessionMessage*, boost::lockfree::capacity<MESSAGE_QUEUE_CAPACITY>>;

class Builder_FIX {
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
    //~Builder_FIX() noexcept;
    
   /*  inline Buffer_FIX *handleResendRequest(const size_t seqnum)
    {
        const auto& msg_history = sess_mngr_.get_history();
        const Buffer_FIX *org_buf = &msg_history[seqnum];
        if (findType(org_buf->data) > 65)
            return build_resend(org_buf);
    }

    inline Buffer_FIX *handleReject()
    {
       return build<FIXTypes::Logout>();
    }

    inline Buffer_FIX *handleTestRequest()
    {
        return build<FIXTypes::Heartbeat>();
    }
 */
    template <FIXTypes T, typename... Args>
    inline Buffer_FIX* build(uint8_t session_index, Args&&... args) noexcept
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
        
        buf_end_ptr = finalizeChecksum(buffer->data, buffer->len);
        buffer->len += 7;
        
        seq_fix.increase_next_seq();
        buffer->hotdata_start = hotdata_temp;
        buffer->hotdata_len = hotdata_len;
        
        return buffer;
    }

    template <FIXTypes T, typename... Args>
    inline char* buildBody(char* buf, uint8_t session_index, Args&&... args) noexcept {
        return buildBody_impl(std::integral_constant<FIXTypes, T>{},
                              buf, session_index, std::forward<Args>(args)...);
    }

    /* template <FIXTypes T, typename... Args>
    inline char *buildBody(char *buf, Args &&...args) noexcept
    {
        return buildBody_impl(std::integral_constant<FIXTypes, T>{},
                              buf, std::forward<Args>(args)...);
    } */

    inline Buffer_FIX* build_resend(const Buffer_FIX* org_buf, uint8_t session_index) noexcept
    {
        auto &auth_fix = sess_mngr_.getSessionAuth(sess_mngr_.getSessionContext(session_index)->tcp_index)->fix;

        Buffer_FIX* temp_buffer = &temp_bufs[temp_index++ & (TEMP_BUFFER_SIZE - 1)];

        const char *hotdata_start = org_buf->hotdata_start;
        const size_t hotdata_len = org_buf->hotdata_len;

        char *buf_end_ptr = buildHeader(org_buf, temp_buffer->data, auth_fix);
        std::memcpy(buf_end_ptr, hotdata_start, hotdata_len);
        temp_buffer->len = static_cast<size_t>(buf_end_ptr + hotdata_len - temp_buffer->data);

        buf_end_ptr = finalizeChecksum(temp_buffer->data, temp_buffer->len);
        temp_buffer->len += 7;

        return temp_buffer;
    }

private:

    // ------------------- HEADER ---------------------
     char* buildHeader(char* buf, size_t body_len, const size_t msg_index, const VenueUserInfo_FIX& auth_fix, Sequence_FIX& seq_fix) noexcept;
     char *buildHeader(const Buffer_FIX *org_buf, char *temp, const VenueUserInfo_FIX &auth_fix) noexcept;

     // ------------------- BODY -------------------------
     // ADMINISTRATIVE MESSAGES
     inline char* buildLogon(char* buf, uint8_t session_index, const bool reset = false) noexcept 
    {
        auto& seq_fix = sess_mngr_.getSessionState(session_index)->fix;
        auto& fix_auth = sess_mngr_.getSessionAuth(sess_mngr_.getSessionContext(session_index)->tcp_index)->fix;

        buf = addTag("98=", 3, "0", 1, buf);
        if(LIKELY(reset))
            buf = addTag("141=", 4, "Y", 1, buf);
        buf = addTag("108=", 4, seq_fix.get_interval(), seq_fix.get_interval_len(), buf);
        buf = addTag("553=", 4, fix_auth.username.data(), fix_auth.username.size(), buf);
        buf = addTag("554=", 4, fix_auth.password.data(), fix_auth.password.size(), buf);
        buf = addTag("1137=", 5, "9", 1, buf);

        return buf;
    }
     inline char* buildLogout(char* buf) noexcept
    {
        return buf;
    }
     inline char* buildHeartbeat(char* buf) noexcept
    {
        return buf;
    }
     inline char* buildTestRequest(char* buf) noexcept
    {
         uint32_t test_req_counter = 1;
        buf = addTag("112=", 4, test_req_counter++, buf);

        return buf;
    }
     inline char* buildSequenceReset(char* buf, const uint32_t new_seq) noexcept
    {
        buf = addTag("123=", 4, "Y", 1, buf);
        buf = addTag("36=", 3, new_seq, buf);

        return buf;
    }
     inline char* buildResendRequest(char* buf, const uint32_t begin_seq, const uint32_t end_seq) noexcept
    {
        buf = addTag("7=", 2, begin_seq, buf);
        buf = addTag("16=", 3, end_seq, buf);

        return buf;
    }
    
    // --- APPLICATION MESSAGES ---
     inline char* buildNewOrderSingle(const Order &order, char* buf, uint8_t session_index) noexcept
    {
        auto &fix_auth = sess_mngr_.getSessionAuth(sess_mngr_.getSessionContext(session_index)->tcp_index)->fix;

        buf = addTag("11=", 3, static_cast<uint32_t>(order.client_order_id), buf);
        buf = addTag("1=", 2, fix_auth.account.data(), fix_auth.account.size(), buf);
        buf = addTag("528=", 4, "A", 1, buf);
        buf = addTag("55=", 3, order.symbol.data(), order.symbol.size() ,buf);
        buf = addTag("54=", 3, static_cast<uint32_t>(order.side), buf);
        
        auto ts_pair = transact_time();
        buf = addTag("60=", 3, ts_pair.first, ts_pair.second, buf);
        
        buf = addTag("38=", 3, order.quantity, buf);
        buf = addTag("40=", 3, static_cast<uint32_t>(order.order_type), buf);

        if(order.order_type == OrderType::Limit)
            buf = addTag("44=", 3, static_cast<uint32_t>(order.price), buf);
        
        buf = addTag("59=", 3, timeinforce[static_cast<size_t>(order.time_in_force)], 1, buf);

        return buf;
    }

     inline char* buildCancelRequest(const Order& order, char* buf) noexcept 
    {
        buf = addTag("41=", 3, "NONE", 4, buf);
        buf = addTag("37=", 3, order.order_id, buf);
        buf = addTag("11=", 3, static_cast<uint32_t>(order.client_order_id), buf);
        buf = addTag("55=", 3, order.symbol.data(), order.symbol.size(), buf);
        buf = addTag("54=", 3, static_cast<uint32_t>(order.side), buf);

        auto ts_pair = transact_time();
        buf = addTag("60=", 3, ts_pair.first, ts_pair.second, buf);

        buf = addTag("38=", 3, order.quantity, buf); 

        return buf;
    }

     inline char* buildCancelReplaceRequest(const Order &order, char* buf, uint8_t session_index) noexcept
    {
        auto &fix_auth = sess_mngr_.getSessionAuth(sess_mngr_.getSessionContext(session_index)->tcp_index)->fix;

        buf = addTag("37=", 3, order.order_id, buf);
        buf = addTag("41=", 3, "NONE", 4, buf);
        buf = addTag("11=", 3, static_cast<uint32_t>(order.client_order_id), buf);
        buf = addTag("1=", 2, fix_auth.account.data(), fix_auth.account.size(), buf);
        buf = addTag("528=", 4, "A", 1, buf);
        buf = addTag("55=", 3, order.symbol.data(), order.symbol.size(), buf);
        buf = addTag("54=", 3, static_cast<uint32_t>(order.side), buf);

        auto ts_pair = transact_time();
        buf = addTag("60=", 3, ts_pair.first, ts_pair.second, buf);

        buf = addTag("38=", 3, order.quantity, buf);
        buf = addTag("40=", 3, static_cast<uint32_t>(order.order_type), buf);

        if (order.order_type == OrderType::Limit)
            buf = addTag("44=", 3, static_cast<uint32_t>(order.price), buf);

        buf = addTag("59=", 3, timeinforce[static_cast<size_t>(order.time_in_force)], 1, buf);

        return buf;
    }

  
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
     inline char *finalizeChecksum(char *buf, size_t len) noexcept
    {
        uint32_t cksum = 0;
        for (size_t i = 0; i < len; ++i)
            cksum += static_cast<unsigned char>(buf[i]);
        cksum %= 256;

        std::memcpy(buf, "10=", 3);
        buf += 3;
        buf[0] = '0' + ((cksum / 100) % 10);
        buf[1] = '0' + ((cksum / 10) % 10);
        buf[2] = '0' + (cksum % 10);
        buf += 3;
        *buf++ = SOH;
        return buf;
    }


   // -------------------  UTIL FUNC --------------------
    static inline std::pair<const char *, const size_t> transact_time() noexcept
    {
        char ts_buf[24];
        const auto now = std::chrono::system_clock::now();
        const std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
        gmtime_r(&t, &tm); 

        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            now.time_since_epoch()) %
                        1000;
        const int written = std::snprintf(
            ts_buf, sizeof(ts_buf),
            "%04d%02d%02d-%02d:%02d:%02d.%03d",
            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
            tm.tm_hour, tm.tm_min, tm.tm_sec,
            static_cast<int>(ms.count()));
        return {ts_buf, static_cast<size_t>(written)};
    }

    static inline char* placeIntValue(uint32_t num, char* buf) noexcept
    {
        static constexpr char DIGIT_PAIRS[200] = {
            '0', '0', '0', '1', '0', '2', '0', '3', '0', '4', '0', '5', '0', '6', '0', '7', '0', '8', '0', '9',
            '1', '0', '1', '1', '1', '2', '1', '3', '1', '4', '1', '5', '1', '6', '1', '7', '1', '8', '1', '9',
            '2', '0', '2', '1', '2', '2', '2', '3', '2', '4', '2', '5', '2', '6', '2', '7', '2', '8', '2', '9',
            '3', '0', '3', '1', '3', '2', '3', '3', '3', '4', '3', '5', '3', '6', '3', '7', '3', '8', '3', '9',
            '4', '0', '4', '1', '4', '2', '4', '3', '4', '4', '4', '5', '4', '6', '4', '7', '4', '8', '4', '9',
            '5', '0', '5', '1', '5', '2', '5', '3', '5', '4', '5', '5', '5', '6', '5', '7', '5', '8', '5', '9',
            '6', '0', '6', '1', '6', '2', '6', '3', '6', '4', '6', '5', '6', '6', '6', '7', '6', '8', '6', '9',
            '7', '0', '7', '1', '7', '2', '7', '3', '7', '4', '7', '5', '7', '6', '7', '7', '7', '8', '7', '9',
            '8', '0', '8', '1', '8', '2', '8', '3', '8', '4', '8', '5', '8', '6', '8', '7', '8', '8', '8', '9',
            '9', '0', '9', '1', '9', '2', '9', '3', '9', '4', '9', '5', '9', '6', '9', '7', '9', '8', '9', '9'};

        /* 0 <= num <= 100  */
        if (UNLIKELY(num < 100))
        {
            if (num < 10)
            {
                *buf++ = '0' + static_cast<char>(num);
                return buf;
            }
            const char* DP = &DIGIT_PAIRS[num * 2];
            *buf++ = DP[0];
            *buf++ = DP[1];
            return buf;
        }

        //clang-format off
        /* num > 100 */
        uint8_t digits =
              (num >= 100000000000000) ? 15 
            : (num >= 10000000000000) ? 14
            : (num >= 1000000000000) ? 13
            : (num >= 100000000000) ? 12
            : (num >= 10000000000) ? 11
            : (num >= 1000000000) ? 10
            : (num >= 100000000) ? 9
            : (num >= 10000000) ? 8
            : (num >= 1000000) ? 7
            : (num >= 100000) ? 6
            : (num >= 10000) ? 5
            : (num >= 1000) ? 4 : 3;
        //clang-format on

        char *end_ptr = buf + digits;
        char *tmp_ptr = end_ptr;

        while (num >= 100)
        {
            const uint32_t quot = num / 100;
            const uint32_t remainder = num - quot * 100;
            num = quot;
            tmp_ptr -= 2;
            const char *DP = &DIGIT_PAIRS[remainder * 2];
            tmp_ptr[0] = DP[0];
            tmp_ptr[1] = DP[1];
        }

        if (num < 10)
            *--tmp_ptr = '0' + static_cast<char>(num);
        else
        {
            tmp_ptr -= 2;
            const char *DP = &DIGIT_PAIRS[num * 2];
            tmp_ptr[0] = DP[0];
            tmp_ptr[1] = DP[1];
        }

        return end_ptr;
    }

    static inline int strnum_to_int(const std::string_view strnum) noexcept
    {
        int result = 0;
        for (const unsigned char c : strnum)
            result = result * 10 + (c - '0');   

        return result;
    }
    static inline uint8_t digit(const auto num)
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

    static inline char findType(const char *data)
    {
        const __m256i eq_mask = _mm256_set1_epi8('=');
        __m256i chunk = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(data));
        __m256i eq_cmp = _mm256_cmpeq_epi8(chunk, eq_mask);
        uint32_t eq_bits = _mm256_movemask_epi8(eq_cmp);
        int eq_offset = 0;

        for (uint8_t i = 0; i < 2; i++)
        {
            eq_offset = __builtin_ctz(eq_bits);
            eq_bits &= ~(1u << eq_offset);
        }

        int third_eq_offset = __builtin_ctz(eq_bits);

        return *(data + third_eq_offset + 1);
    }
    static inline std::pair<const char *, size_t> findValue(const char *data, size_t len, std::string_view wantTag) noexcept
    {
        constexpr char SOH = '\x01';

        const __m256i eq_mask = _mm256_set1_epi8('=');
        const __m256i soh_mask = _mm256_set1_epi8(SOH);

        size_t pos = 0;
        size_t tag_start = 0;

        while (pos + 32 <= len)
        {
            __m256i chunk = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(data + pos));

            uint32_t eq_bits = _mm256_movemask_epi8(_mm256_cmpeq_epi8(chunk, eq_mask));
            uint32_t soh_bits = _mm256_movemask_epi8(_mm256_cmpeq_epi8(chunk, soh_mask));

            while (eq_bits != 0)
            {
                int eq_offset = __builtin_ctz(eq_bits);
                size_t eq_pos = pos + eq_offset;

                // find next SOH
                uint32_t soh_after = soh_bits & (~0u << (eq_offset + 1));
                size_t soh_pos;

                if (soh_after != 0)
                {
                    int soh_offset = __builtin_ctz(soh_after);
                    soh_pos = pos + soh_offset;
                }
                else
                {
                    size_t q = eq_pos + 1;
                    while (q < len && data[q] != SOH)
                        ++q;
                    soh_pos = q;
                }

                // tag = [tag_start, eq_pos)
                size_t tag_len = eq_pos - tag_start;

                if (tag_len == wantTag.size() &&
                    memcmp(data + tag_start, wantTag.data(), tag_len) == 0)
                {
                    const char *val = data + eq_pos + 1;
                    size_t vlen = soh_pos - (eq_pos + 1);
                    return {val, vlen};
                }

                tag_start = soh_pos + 1;
                eq_bits &= ~(1u << eq_offset);
            }

            pos += 32;
        }

        // scalar tail
        while (pos < len)
        {
            size_t eq_pos = pos;
            while (eq_pos < len && data[eq_pos] != '=')
                ++eq_pos;
            if (eq_pos >= len)
                break;

            size_t tag_len = eq_pos - tag_start;

            // find SOH
            size_t soh_pos = eq_pos + 1;
            while (soh_pos < len && data[soh_pos] != SOH)
                ++soh_pos;

            // MATCH
            if (tag_len == wantTag.size() &&
                memcmp(data + tag_start, wantTag.data(), tag_len) == 0)
            {
                const char *val = data + eq_pos + 1;
                size_t vlen = soh_pos - (eq_pos + 1);
                return {val, vlen};
            }

            tag_start = soh_pos + 1;
            pos = soh_pos + 1;
        }

        return {nullptr, 0};
    }
};

