#include "Builder_FIX.h"
#include "common.h"

#include <immintrin.h>
#include <chrono>
#include <cstdio>
#include <ctime>

Builder_FIX::Builder_FIX(SessionManager &sess_mngr) noexcept : sess_mngr_(sess_mngr)
{  }

Buffer_FIX *Builder_FIX::build_resend(const Buffer_FIX *org_buf, uint8_t session_index) noexcept
{
    auto &auth_fix = sess_mngr_.getSessionAuth(sess_mngr_.getSessionContext(session_index)->tcp_index)->fix;

    Buffer_FIX *temp_buffer = &temp_bufs[temp_index++ & (TEMP_BUFFER_SIZE - 1)];

    const char *hotdata_start = org_buf->hotdata_start;
    const size_t hotdata_len = org_buf->hotdata_len;

    char *buf_end_ptr = buildHeader(org_buf, temp_buffer->data, auth_fix);
    std::memcpy(buf_end_ptr, hotdata_start, hotdata_len);
    temp_buffer->len = static_cast<size_t>(buf_end_ptr + hotdata_len - temp_buffer->data);

    buf_end_ptr = finalizeChecksum(temp_buffer->data, temp_buffer->len);
    temp_buffer->len += 7;

    return temp_buffer;
}

char *Builder_FIX::buildHeader(char *buf, size_t body_len, const size_t msg_index, const VenueUserInfo_FIX &auth_fix, Sequence_FIX &seq_fix) noexcept
{
    const uint32_t next_seqnum = seq_fix.get_next_seq();
    const uint8_t seqnum_digits = digit(next_seqnum);

    auto ts_pair = transact_time();
    body_len += 1 + auth_fix.my_id.size() + auth_fix.venue_id.size() + seqnum_digits + ts_pair.second + 20;
    
    buf = addTag("8=", 2, "FIX.4.4", 7, buf);
    buf = addTag("9=", 2, static_cast<uint32_t>(body_len), buf);
    buf = addTag("35=", 3, msg_types[msg_index], 1, buf);
    buf = addTag("49=", 3, auth_fix.my_id.data(), auth_fix.my_id.size(), buf);   
    buf = addTag("56=", 3, auth_fix.venue_id.data(), auth_fix.venue_id.size(), buf); 
    buf = addTag("34=", 3, next_seqnum, buf);
    buf = addTag("52=", 3, ts_pair.first, ts_pair.second, buf);

    return buf;
}

char *Builder_FIX::buildHeader(const Buffer_FIX *org_buf, char *temp, const VenueUserInfo_FIX &auth_fix) noexcept
{
    const char* data = org_buf->data;
    const size_t buf_len = org_buf->len;
   
    auto [type, type_len] = findValue(data, buf_len, "35");
    auto [seq, seq_len] = findValue(data, buf_len, "34");
    auto [org_send_time, org_send_time_len] = findValue(data, buf_len, "52");
    auto [body_len_str, body_len_str_len] = findValue(data, buf_len, "9");
    
    size_t body_len = static_cast<size_t>((strnum_to_int(body_len_str)));
    auto ts_pair = transact_time();
    body_len += 10 + ts_pair.second;

    temp = addTag("8=", 2, "FIX.4.4", 7, temp);
    temp = addTag("9=", 2, body_len, temp);
    temp = addTag("35=", 3, type, 1, temp);
    temp = addTag("49=", 3, auth_fix.my_id.data(), auth_fix.my_id.size(), temp);
    temp = addTag("56=", 3, auth_fix.venue_id.data(), auth_fix.venue_id.size(), temp);
    temp = addTag("34=", 3, seq, seq_len, temp);
    temp = addTag("43=", 3, "Y", 1, temp);
    temp = addTag("122=", 4, org_send_time, org_send_time_len, temp);
    temp = addTag("52=", 3, ts_pair.first, ts_pair.second, temp);

    return temp;
}

char* Builder_FIX::buildLogon(char *buf, uint8_t session_index, const bool reset) noexcept
{
    auto &seq_fix = sess_mngr_.getSessionState(session_index)->fix;
    auto &fix_auth = sess_mngr_.getSessionAuth(sess_mngr_.getSessionContext(session_index)->tcp_index)->fix;

    buf = addTag("98=", 3, "0", 1, buf);
    if (LIKELY(reset))
        buf = addTag("141=", 4, "Y", 1, buf);
    buf = addTag("108=", 4, seq_fix.get_interval(), seq_fix.get_interval_len(), buf);
    buf = addTag("553=", 4, fix_auth.username.data(), fix_auth.username.size(), buf);
    buf = addTag("554=", 4, fix_auth.password.data(), fix_auth.password.size(), buf);
    buf = addTag("1137=", 5, "9", 1, buf);

    return buf;
}
char* Builder_FIX::buildTestRequest(char *buf) noexcept
{
    uint32_t test_req_counter = 1;
    buf = addTag("112=", 4, test_req_counter++, buf);

    return buf;
}
char* Builder_FIX::buildSequenceReset(char *buf, const uint32_t new_seq) noexcept
{
    buf = addTag("123=", 4, "Y", 1, buf);
    buf = addTag("36=", 3, new_seq, buf);

    return buf;
}
char* Builder_FIX::buildResendRequest(char *buf, const uint32_t begin_seq, const uint32_t end_seq) noexcept
{
    buf = addTag("7=", 2, begin_seq, buf);
    buf = addTag("16=", 3, end_seq, buf);

    return buf;
}

char* Builder_FIX::buildNewOrderSingle(const Order &order, char *buf, uint8_t session_index) noexcept
{
    auto &fix_auth = sess_mngr_.getSessionAuth(sess_mngr_.getSessionContext(session_index)->tcp_index)->fix;
        
    buf = addTag("11=", 3, order.client_order_token.data(), 13, buf);
    buf = addTag("1=", 2, fix_auth.account.data(), fix_auth.account.size(), buf);
    buf = addTag("528=", 4, "A", 1, buf);
    buf = addTag("55=", 3, order.symbol.data(), order.real_symbol_len, buf);
    buf = addTag("54=", 3, static_cast<uint32_t>(order.side), buf);

    auto ts_pair = epoch_to_transact_time(order.timestamp);
    buf = addTag("60=", 3, ts_pair.first, ts_pair.second, buf);

    buf = addTag("38=", 3, order.quantity, buf);
    buf = addTag("40=", 3, static_cast<uint32_t>(order.order_type), buf);

    if (order.order_type == OrderType::Limit)
        buf = addTagPrice<4>("44=", 3, static_cast<uint32_t>(order.price), buf);

    buf = addTag("59=", 3, timeinforce[static_cast<size_t>(order.time_in_force)], 1, buf);

    return buf;
}

char* Builder_FIX::buildCancelRequest(const Order &order, char *buf) noexcept
{
    buf = addTag("41=", 3, "NONE", 4, buf);
    buf = addTag("37=", 3, order.order_id, buf);
    buf = addTag("11=", 3, order.client_order_token.data(), order.real_cl_ord_token_len, buf);
    buf = addTag("55=", 3, order.symbol.data(), order.real_symbol_len, buf);
    buf = addTag("54=", 3, static_cast<uint32_t>(order.side), buf);

    auto ts_pair = epoch_to_transact_time(order.timestamp);    
    buf = addTag("60=", 3, ts_pair.first, ts_pair.second, buf);

    buf = addTag("38=", 3, order.quantity, buf);

    return buf;
}

char* Builder_FIX::buildCancelReplaceRequest(const Order &order, char *buf, uint8_t session_index) noexcept
{
    auto &fix_auth = sess_mngr_.getSessionAuth(sess_mngr_.getSessionContext(session_index)->tcp_index)->fix;

    buf = addTag("37=", 3, order.order_id, buf);
    buf = addTag("41=", 3, "NONE", 4, buf);
    buf = addTag("11=", 3, order.client_order_token.data(), order.real_cl_ord_token_len, buf);
    buf = addTag("1=", 2, fix_auth.account.data(), fix_auth.account.size(), buf);
    buf = addTag("528=", 4, "A", 1, buf);
    buf = addTag("55=", 3, order.symbol.data(), order.real_symbol_len, buf);
    buf = addTag("54=", 3, static_cast<uint32_t>(order.side), buf);

    auto ts_pair = epoch_to_transact_time(order.timestamp);
    buf = addTag("60=", 3, ts_pair.first, ts_pair.second, buf);

    buf = addTag("38=", 3, order.quantity, buf);
    buf = addTag("40=", 3, static_cast<uint32_t>(order.order_type), buf);

    if (order.order_type == OrderType::Limit)
        buf = addTagPrice<4>("44=", 3, static_cast<uint32_t>(order.price), buf);

    buf = addTag("59=", 3, timeinforce[static_cast<size_t>(order.time_in_force)], 1, buf);

    return buf;
}

char* Builder_FIX::finalizeChecksum(char *buf, size_t len) noexcept
{
    uint32_t cksum = 0;
    for (size_t i = 0; i < len; ++i)
        cksum += static_cast<unsigned char>(buf[i]);
    cksum %= 256;

    std::memcpy(buf+len, "10=", 3);
    buf += len + 3;
    buf[0] = '0' + ((cksum / 100) % 10);
    buf[1] = '0' + ((cksum / 10) % 10);
    buf[2] = '0' + (cksum % 10);
    buf += 3;
    *buf++ = SOH;
    return buf;
}

// std::pair<const char *, const size_t> Builder_FIX::transact_time() noexcept
// {
//     static thread_local char ts_buf[32];

//     using namespace std::chrono;

//     const auto now = system_clock::now();
//     const std::time_t t = system_clock::to_time_t(now);

//     std::tm tm{};
//     gmtime_r(&t, &tm);

//     const auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
    
//     const int written = std::snprintf(
//         ts_buf, sizeof(ts_buf),
//         "%04d%02d%02d-%02d:%02d:%02d.%03d",
//         tm.tm_year + 1900, 
//         tm.tm_mon + 1, 
//         tm.tm_mday,
//         tm.tm_hour, 
//         tm.tm_min, 
//         tm.tm_sec,
//         static_cast<int>(ms.count())
//     );

//     return {ts_buf, static_cast<size_t>(written)};
// }

// static inline std::pair<const char*, size_t> epoch_to_transact_time(uint64_t ns) noexcept
// {
//     static thread_local char ts_buf[32];

//     using namespace std::chrono;

//     const system_clock::time_point tp{nanoseconds(ns)};
//     const std::time_t t = system_clock::to_time_t(tp);

//     std::tm tm{};
//     gmtime_r(&t, &tm);

//     const uint32_t ms = (ns / 1'000'000ULL) % 1000;

//     const int written = std::snprintf(
//         ts_buf, sizeof(ts_buf),
//         "%04d%02d%02d-%02d:%02d:%02d.%03u",
//         tm.tm_year + 1900,
//         tm.tm_mon + 1,
//         tm.tm_mday,
//         tm.tm_hour,
//         tm.tm_min,
//         tm.tm_sec,
//         ms
//     );

//     return {ts_buf, static_cast<size_t>(written)};
// }

std::pair<const char*, const size_t> Builder_FIX::transact_time() noexcept
{
    static thread_local char ts_buf[32];

    using namespace std::chrono;

    const auto now = system_clock::now();
    const std::time_t t = system_clock::to_time_t(now);

    std::tm tm{};
    localtime_r(&t, &tm);

    const auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    char* p = ts_buf;

    // YYYY
    const int y = tm.tm_year + 1900;
    *p++ = '0' + (y / 1000) % 10;
    *p++ = '0' + (y / 100) % 10;
    *p++ = '0' + (y / 10) % 10;
    *p++ = '0' + (y % 10);

    // MM
    const int mo = tm.tm_mon + 1;
    *p++ = '0' + (mo / 10);
    *p++ = '0' + (mo % 10);

    // DD
    const int d = tm.tm_mday;
    *p++ = '0' + (d / 10);
    *p++ = '0' + (d % 10);

    *p++ = '-';

    // HH
    const int h = tm.tm_hour;
    *p++ = '0' + (h / 10);
    *p++ = '0' + (h % 10);

    *p++ = ':';

    // MM
    const int m = tm.tm_min;
    *p++ = '0' + (m / 10);
    *p++ = '0' + (m % 10);

    *p++ = ':';

    // SS
    const int s = tm.tm_sec;
    *p++ = '0' + (s / 10);
    *p++ = '0' + (s % 10);

    *p++ = '.';

    // mmm
    const int milli = static_cast<int>(ms.count());
    *p++ = '0' + (milli / 100);
    *p++ = '0' + (milli / 10) % 10;
    *p++ = '0' + (milli % 10);

    return {ts_buf, static_cast<size_t>(p - ts_buf)};
}

std::pair<const char*, const size_t> Builder_FIX::epoch_to_transact_time(uint64_t ns) noexcept
{
    static thread_local char ts_buf[32];

    using namespace std::chrono;

    const system_clock::time_point tp{nanoseconds(ns)};
    const std::time_t t = system_clock::to_time_t(tp);

    std::tm tm{};
    localtime_r(&t, &tm);

    const uint32_t ms = (ns / 1'000'000ULL) % 1000;

    char* p = ts_buf;

    // YYYY
    int y = tm.tm_year + 1900;
    *p++ = '0' + (y / 1000) % 10;
    *p++ = '0' + (y / 100) % 10;
    *p++ = '0' + (y / 10) % 10;
    *p++ = '0' + (y % 10);

    // MM
    int mo = tm.tm_mon + 1;
    *p++ = '0' + (mo / 10);
    *p++ = '0' + (mo % 10);

    // DD
    int d = tm.tm_mday;
    *p++ = '0' + (d / 10);
    *p++ = '0' + (d % 10);

    *p++ = '-';

    // HH
    int h = tm.tm_hour;
    *p++ = '0' + (h / 10);
    *p++ = '0' + (h % 10);

    *p++ = ':';

    // MM
    int m = tm.tm_min;
    *p++ = '0' + (m / 10);
    *p++ = '0' + (m % 10);

    *p++ = ':';

    // SS
    int s = tm.tm_sec;
    *p++ = '0' + (s / 10);
    *p++ = '0' + (s % 10);

    *p++ = '.';

    // ms
    *p++ = '0' + (ms / 100);
    *p++ = '0' + (ms / 10) % 10;
    *p++ = '0' + (ms % 10);

    return {ts_buf, static_cast<size_t>(p - ts_buf)};
}

char* Builder_FIX::placeIntValue(uint32_t num, char *buf) noexcept
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
        const char *DP = &DIGIT_PAIRS[num * 2];
        *buf++ = DP[0];
        *buf++ = DP[1];
        return buf;
    }

    //clang-format off
    /* num > 100 */
    uint8_t digits =
        (num >= 1000000000)     ? 10
        : (num >= 100000000)      ? 9
        : (num >= 10000000)       ? 8
        : (num >= 1000000)        ? 7
        : (num >= 100000)         ? 6
        : (num >= 10000)          ? 5
        : (num >= 1000)           ? 4
                                  : 3;
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

char Builder_FIX::findType(const char *data) noexcept
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

std::pair<const char *, size_t> Builder_FIX::findValue(const char *data, size_t len, std::string_view wantTag) noexcept
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

    while (pos < len)
    {
        size_t eq_pos = pos;
        while (eq_pos < len && data[eq_pos] != '=')
            ++eq_pos;
        if (eq_pos >= len)
            break;

        size_t tag_len = eq_pos - tag_start;

        size_t soh_pos = eq_pos + 1;
        while (soh_pos < len && data[soh_pos] != SOH)
            ++soh_pos;

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