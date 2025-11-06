#include "Builder_FIX.h"

Builder_FIX::Builder_FIX(Session_FIX &session) noexcept : session_(session), vui_(session_.get_vui())
{  }

char* Builder_FIX::buildHeader(Builder_FIX &bf, char* buf, size_t body_len, const size_t msg_index) noexcept 
{
    const uint32_t next_seqnum = bf.session_.get_next_seq();
    const uint8_t seqnum_digits = digit(next_seqnum);

    auto ts_pair = transact_time();
    body_len += 1 + bf.vui_.my_id_len + bf.vui_.venue_id_len + seqnum_digits + ts_pair.second + 20;
    
    buf = addTag("8=", 2, "9", 1, buf);
    buf = addTag("9=", 2, static_cast<uint32_t>(body_len), buf);
    buf = addTag("35=", 3, msg_types[msg_index], 1, buf);
    buf = addTag("49=", 3, bf.vui_.my_id, bf.vui_.my_id_len, buf);   
    buf = addTag("56=", 3, bf.vui_.venue_id, bf.vui_.venue_id_len, buf); 
    buf = addTag("34=", 3, next_seqnum, buf);
    buf = addTag("52=", 3, ts_pair.first, ts_pair.second, buf);

    return buf;
}

char* Builder_FIX::buildHeader(Builder_FIX &bf, const Buffer* org_buf, char* temp) noexcept
{
    const char* data = org_buf-> data;
    const size_t buf_len = org_buf->len;
   
    auto [type, type_len] = findValue(data, buf_len, "35");
    auto [seq, seq_len] = findValue(data, buf_len, "34");
    auto [org_send_time, org_send_time_len] = findValue(data, buf_len, "52");
    auto [body_len_str, body_len_str_len] = findValue(data, buf_len, "9");
    
    size_t body_len = static_cast<size_t>((strnum_to_int(body_len_str)));
    auto ts_pair = transact_time();
    body_len += 10 + ts_pair.second;

    temp = addTag("8=", 2, "9", 1, temp);
    temp = addTag("9=", 2, body_len, temp);
    temp = addTag("35=", 3, type, 1, temp);
    temp = addTag("49=", 3, bf.vui_.my_id, bf.vui_.my_id_len, temp);
    temp = addTag("56=", 3, bf.vui_.venue_id, bf.vui_.venue_id_len, temp);
    temp = addTag("34=", 3, seq, seq_len, temp);
    temp = addTag("43=", 3, "Y", 1, temp);
    temp = addTag("122=", 4, org_send_time, org_send_time_len, temp);
    temp = addTag("52=", 3, ts_pair.first, ts_pair.second, temp);

    return temp;
}
