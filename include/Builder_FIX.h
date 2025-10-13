/* static constexpr size_t BUFFER_POOL_CAPACITY = 1024;
 std::array<Buffer,BUFFER_POOL_CAPACITY> buffer_pool_;
    std::vector<Buffer*> free_buffers_;
      for (size_t i = 0; i < BUFFER_POOL_CAPACITY; i++)
            free_buffers_.push_back(&buffer_pool_[i]); */

#pragma once

#include "common.h"
#include "Order.h"

#include <chrono>
#include <array>
#include <cstring>
#include <cstdint>
#include <cstdio>

struct VenueID
{
    const char* my_id;
    const char* venue_id;
    const size_t my_id_len;
    const size_t venue_id_len;
};

struct alignas(64) Buffer 
{
    static constexpr size_t MAX_MSG_SIZE = 512;

    char data[MAX_MSG_SIZE];
    size_t len;
};

class Builder_FIX {
private:
    static constexpr char SOH = '\x01';
    static constexpr char EQ  = '\x3D';

    static constexpr std::array<VenueID, VENUE_COUNT> venueIDs
    {
        VenueID{"BIST MY ID", "BIST ID", 0, 0 },
        VenueID{"NYSE MY ID", "NYSE ID", 0, 0 },
        VenueID{"NASDAQ MY ID", "NASDAQ ID", 0, 0},
    };

    static std::array<uint32_t, VENUE_COUNT> seq_nums;
     
public:

    inline Buffer& build(Order& order, Buffer& buffer) noexcept 
    {    
        char body_temp[256];
        const char msgType = order.message_type;  // örn. 'D', 'F', 'G'
        auto handler = buildHandlers[static_cast<unsigned char>(msgType)];
        if (LIKELY(handler)) 
        {
            char* body_end_ptr = handler(order, body_temp);
            size_t body_len = static_cast<size_t>(body_end_ptr - body_temp);
            char* buf_end_ptr = buildHeader(buffer.data, body_len, order.venue);
            std::memcpy(buf_end_ptr, body_temp, body_len);
            buffer.len = static_cast<size_t>(buf_end_ptr + body_len - buffer.data);  // before chksum len
            buf_end_ptr = finalizeChecksum(buffer.data, buffer.len);
            buffer.len += 7;  // after chcksum len
        }
        return buffer; 
    }

private:
 
    // hızlı integer to ascii (branchless)
    static inline char* fast_itoa(uint32_t num, char* buf) noexcept 
    {
        static constexpr char DIGIT_PAIRS[200] = {
            '0','0','0','1','0','2','0','3','0','4','0','5','0','6','0','7','0','8','0','9',
            '1','0','1','1','1','2','1','3','1','4','1','5','1','6','1','7','1','8','1','9',
            '2','0','2','1','2','2','2','3','2','4','2','5','2','6','2','7','2','8','2','9',
            '3','0','3','1','3','2','3','3','3','4','3','5','3','6','3','7','3','8','3','9',
            '4','0','4','1','4','2','4','3','4','4','4','5','4','6','4','7','4','8','4','9',
            '5','0','5','1','5','2','5','3','5','4','5','5','5','6','5','7','5','8','5','9',
            '6','0','6','1','6','2','6','3','6','4','6','5','6','6','6','7','6','8','6','9',
            '7','0','7','1','7','2','7','3','7','4','7','5','7','6','7','7','7','8','7','9',
            '8','0','8','1','8','2','8','3','8','4','8','5','8','6','8','7','8','8','8','9',
            '9','0','9','1','9','2','9','3','9','4','9','5','9','6','9','7','9','8','9','9'
        };

        /* 0 <= num <= 100  */
        if (UNLIKELY(num < 100)) {
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

        /* num > 100 */
        uint8_t digits =
            (num >= 100000000000000) ? 15 :
            (num >= 10000000000000) ? 14 :
            (num >= 1000000000000) ? 13 :
            (num >= 100000000000) ? 12 :
            (num >= 10000000000) ? 11 :
            (num >= 1000000000) ? 10 :
            (num >= 100000000) ? 9 :
            (num >= 10000000) ? 8 :
            (num >= 1000000) ? 7 :
            (num >= 100000) ? 6 :
            (num >= 10000) ? 5 :
            (num >= 1000) ? 4 : 3;

        char* end_ptr = buf + digits;
        char* tmp_ptr = end_ptr;

        while (num >= 100) {
            const uint32_t quot = num / 100;
            const uint32_t remainder = num - quot * 100;
            num = quot;
            tmp_ptr -= 2;
            const char* DP = &DIGIT_PAIRS[remainder * 2];
            tmp_ptr[0] = DP[0];
            tmp_ptr[1] = DP[1];
        }

        if (num < 10)
            *--tmp_ptr = '0' + static_cast<char>(num);
        else 
        {
            tmp_ptr -= 2;
            const char* DP = &DIGIT_PAIRS[num * 2];
            tmp_ptr[0] = DP[0]; 
            tmp_ptr[1] = DP[1];
        }

        return end_ptr;
    }

    static inline char* addTag(int tag, const char* val, const size_t len, char* buf) noexcept 
    {
        buf = fast_itoa(tag, buf);
        *buf++ = EQ;
        std::memcpy(buf, val, len);
        buf += len;
        *buf++ = SOH;
        return buf;
    }

    static inline char* addTag(int tag, uint32_t val, char* buf) noexcept 
    {
        buf = fast_itoa(tag, buf);
        *buf++ = EQ;
        buf = fast_itoa(val, buf);
        *buf++ = SOH;
        return buf;
    }

    static char* buildHeader(char* buf, size_t body_len, Venue venue) noexcept; 

    // --- message builders ---
    static inline char* buildNewOrderSingle(const Order& order, char* buf) noexcept 
    {
        // örnek alanlar: 35=DP, 11=ClOrdID, 55=Symbol, 54=Side, 38=Qty, 44=Price
        buf = addTag(35, "D", 1, buf);
        buf = addTag(11, static_cast<uint32_t>(order.client_order_id), buf);
        buf = addTag(55, order.symbol.data(), order.symbol.size() ,buf);
        buf = addTag(54, static_cast<uint32_t>(order.side), buf);
        buf = addTag(38, order.quantity, buf);
        buf = addTag(44, static_cast<uint32_t>(order.price), buf);

        return buf;
    }

    static inline char* buildCancelRequest(const Order& order, char* buf) noexcept 
    {
        // örnek alanlar: 35=F, 41=OrigClOrdID, 11=ClOrdID, 55=Symbol, 54=Side
        buf = addTag(35, "F", 1, buf);
        buf = addTag(41, order.order_id, buf);
        buf = addTag(11, static_cast<uint32_t>(order.client_order_id), buf);
        buf = addTag(55, order.symbol.data(), order.symbol.size(), buf);
        buf = addTag(54, static_cast<uint32_t>(order.side), buf);

        return buf;
    }

    static inline char* buildReplaceRequest(const Order& order, char* buf) noexcept 
    {
        // örnek alanlar: 35=G, 41=OrigClOrdID, 11=ClOrdID, 55=Symbol, 38=Qty, 44=Price
        buf = addTag(35, "G", 1, buf);
        buf = addTag(41, static_cast<uint32_t>(order.order_id), buf);
        buf = addTag(11, static_cast<uint32_t>(order.client_order_id), buf);
        buf = addTag(55, order.symbol.data(), order.symbol.size(), buf);
        buf = addTag(38, order.quantity, buf);
        buf = addTag(44, static_cast<uint32_t>(order.price), buf);

        return buf;
    }

   static inline char* finalizeChecksum(char* buf, size_t len) noexcept {
    uint32_t cksum = 0;
    for (size_t i = 0; i < len; ++i)
        cksum += static_cast<unsigned char>(buf[i]);
    cksum %= 256;

    buf = fast_itoa(10, buf);
    *buf++ = EQ;
    buf[0] = '0' + ((cksum / 100) % 10);
    buf[1] = '0' + ((cksum / 10) % 10);
    buf[2] = '0' + (cksum % 10);
    buf += 3;
    *buf++ = SOH;
    return buf;
}


    using BuildHandlerFunc = char* (*)(const Order&, char*) noexcept;
    inline static constexpr auto buildHandlers = [] {
        std::array<BuildHandlerFunc, 128> arr{};
        arr['D'] = &Builder_FIX::buildNewOrderSingle;
        arr['F'] = &Builder_FIX::buildCancelRequest;
        arr['G'] = &Builder_FIX::buildReplaceRequest;
        return arr;
    }();
};


