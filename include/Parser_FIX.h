#pragma once

#include "common.h"

#include <string_view>
#include <cstdint>
#include <array>
#include <cstring>
#include <vector>
#include <immintrin.h>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/pool/object_pool.hpp>

struct alignas(64) FIXMessage
{
    int64_t price = -1;         // 8 byte, FIX Tag 44: Fiyat (fixed-point encoding)
    uint32_t quantity = 0;      // 4 byte, FIX Tag 38: Miktar (Order Qty)
    uint32_t leaves_qty = 0;    // 4 byte, FIX Tag 151: Kalan miktar (Remaining Qty)
    uint32_t last_qty = 0;      // 4 byte, FIX Tag 14: Doldurulan miktar (Exec Qty)
    uint32_t filled_qty = 0;    // 4 byte, FIX Tag 32: Doldurulan miktar (Filled Qty (cum))
    uint32_t transact_time = 0; // 4 byte, FIX Tag 60: İşlem zamanı (UTC timestamp saniye cinsinden)

    uint8_t msg_type = 0;      // 1 byte, FIX Tag 35: Mesaj tipi (NewOrder, ExecReport, vb.)
    uint8_t side = 0;          // 1 byte, FIX Tag 54: Emir yönü (Buy/Sell)
    uint8_t ord_status = 0;    // 1 byte, FIX Tag 39: Emir durumu (New, Filled, Cancelled, vb.)
    uint8_t exec_type = 0;     // 1 byte, FIX Tag 150: Gerçekleşme tipi (Trade, Cancel, vb.)
    uint8_t ord_type = 0;      // 1 byte, FIX Tag 40: Emir tipi (Limit, Market, vb.)
    uint8_t time_in_force = 0; // 1 byte, FIX Tag 59: Geçerlilik süresi

    uint8_t pad1[14]; // 2 byte padding (8-byte hizalamayı sağlamak için)
    std::string_view symbol; // 16 byte, FIX Tag 55: İşlem gören varlık (Symbol)

    //cache-line boundary (64 byte)
    std::string_view order_id;  // 16 byte, FIX Tag 37: Broker tarafından atanan emir ID'si (Order ID)
    std::string_view cl_ord_id; // 16 byte, FIX Tag 11: Müşteri emir ID'si (Client Order ID)
    std::string_view exec_id;   // 16 byte, FIX Tag 17: Gerçekleşme ID'si (Execution ID)
    std::string_view fix_version;
};

inline constexpr size_t FIX_QUEUE_CAPACITY = 1024;

using FIXMessagePool = std::vector<FIXMessage>;
using spscFIXQueue_t = boost::lockfree::spsc_queue<FIXMessage *, boost::lockfree::capacity<FIX_QUEUE_CAPACITY>>;

class Parser_FIX
{
private:
    FIXMessagePool fixMsg_pool_;
    spscFIXQueue_t free_fixMsg_list_;

    static constexpr char SOH = '\x01';
    static constexpr size_t MAX_TAG = 192;

    using TagHandlerFunc = void (*)(std::string_view, FIXMessage *) noexcept;
    static std::array<TagHandlerFunc, MAX_TAG> makeTagHandlersLookup() noexcept;
    static std::array<TagHandlerFunc, MAX_TAG> tagHandlers;

public:
    Parser_FIX() noexcept;

    inline void releaseFIX(FIXMessage *fixMsg) noexcept
    {
        free_fixMsg_list_.push(fixMsg);
    }

    FIXMessage *parse(const char *data, size_t len) noexcept;

private:

    static inline int parseFixedPoint(std::string_view strnum) noexcept
    {
        int result = 0;
        for (char c : strnum)
        {
            if (LIKELY(c >= '0' && c <= '9'))
            {
                result = result * 10 + (c - '0');
            }
        }
        return result;
    }

    static inline uint8_t parseNumber(const char *str, size_t len) noexcept
    {
        if(!isdigit(str[0])) 
            return static_cast<uint8_t>(str[0]);

        uint8_t result = 0;
        for (uint8_t i = 0; i < len; ++i)
        {
            char c = str[i];
            result = result * 10 + (c - '0');
        }
        return result;
    }

};
