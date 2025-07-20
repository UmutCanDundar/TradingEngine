#pragma once

#include "BaseParser.h"

#include <string_view>
#include <cstdint>
#include <array>
#include <cstring>
#include <immintrin.h>

struct alignas(64) FIXMessage
{
    char msg_type = '\0';      // 1 byte, FIX Tag 35: Mesaj tipi (NewOrder, ExecReport, vb.)
    char side = '\0';          // 1 byte, FIX Tag 54: Emir yönü (Buy/Sell)
    char ord_status = '\0';    // 1 byte, FIX Tag 39: Emir durumu (New, Filled, Cancelled, vb.)
    char exec_type = '\0';     // 1 byte, FIX Tag 150: Gerçekleşme tipi (Trade, Cancel, vb.)
    char ord_type = '\0';      // 1 byte, FIX Tag 40: Emir tipi (Limit, Market, vb.)
    char time_in_force = '\0'; // 1 byte, FIX Tag 59: Geçerlilik süresi

    char pad1[2]; // 2 byte padding (8-byte hizalamayı sağlamak için)

    int64_t price = -1;         // 8 byte, FIX Tag 44: Fiyat (fixed-point encoding)
    uint32_t quantity = 0;      // 4 byte, FIX Tag 38: Miktar (Order Qty)
    uint32_t leaves_qty = 0;    // 4 byte, FIX Tag 151: Kalan miktar (Remaining Qty)
    uint32_t filled_qty = 0;    // 4 byte, FIX Tag 32: Doldurulan miktar (Filled Qty)
    uint32_t transact_time = 0; // 4 byte, FIX Tag 60: İşlem zamanı (UTC timestamp saniye cinsinden)

    std::string_view symbol;   // 16 byte, FIX Tag 55: İşlem gören varlık (Symbol)
    std::string_view order_id; // 16 byte, FIX Tag 37: Broker tarafından atanan emir ID'si (Order ID)

    std::string_view fix_version;
    std::string_view cl_ord_id; // 16 byte, FIX Tag 11: Müşteri emir ID'si (Client Order ID)
    std::string_view exec_id;   // 16 byte, FIX Tag 17: Gerçekleşme ID'si (Execution ID)

    char pad2[16]; // 16 byte padding
};

class FIXParser
{
private:
    static constexpr char SOH = '\x01';
    static constexpr size_t MAX_TAG = 192;

    static inline int parseFixedPoint(std::string_view strnum) noexcept
    {
        int result = 0;
        for (char c : strnum)
        {
            if (c >= '0' && c <= '9')
            {
                result = result * 10 + (c - '0');
            }
        }
        return result;
    }
    using TagHandlerFunc = void (*)(std::string_view, FIXMessage &) noexcept;

    static inline std::array<TagHandlerFunc, MAX_TAG> makeTagHandlersLookup() noexcept
    {
        std::array<TagHandlerFunc, MAX_TAG> handlers{};
        // clang-format off
        handlers[8]  = [](std::string_view val, FIXMessage &msg) noexcept { msg.fix_version = val; };
        handlers[35] = [](std::string_view val, FIXMessage &msg) noexcept { msg.msg_type = val[0]; };
        handlers[44] = [](std::string_view val, FIXMessage &msg) noexcept { msg.price = parseFixedPoint(val); };
        handlers[38] = [](std::string_view val, FIXMessage &msg) noexcept { msg.quantity = static_cast<uint32_t>(parseFixedPoint(val)); };
        handlers[151] = [](std::string_view val, FIXMessage &msg) noexcept { msg.leaves_qty = static_cast<uint32_t>(parseFixedPoint(val)); };
        handlers[32] = [](std::string_view val, FIXMessage &msg) noexcept { msg.filled_qty = static_cast<uint32_t>(parseFixedPoint(val)); };
        handlers[60] = [](std::string_view val, FIXMessage &msg) noexcept { msg.transact_time = static_cast<uint32_t>(parseFixedPoint(val)); };
        handlers[54] = [](std::string_view val, FIXMessage &msg) noexcept { msg.side = val[0]; };
        handlers[40] = [](std::string_view val, FIXMessage &msg) noexcept { msg.ord_type = val[0]; };
        handlers[59] = [](std::string_view val, FIXMessage &msg) noexcept { msg.time_in_force = val[0]; };
        handlers[39] = [](std::string_view val, FIXMessage &msg) noexcept { msg.ord_status = val[0]; };
        handlers[150] = [](std::string_view val, FIXMessage &msg) noexcept { msg.exec_type = val[0]; };
        handlers[55] = [](std::string_view val, FIXMessage &msg) noexcept { msg.symbol = val; };
        handlers[11] = [](std::string_view val, FIXMessage &msg) noexcept { msg.cl_ord_id = val; };
        handlers[37] = [](std::string_view val, FIXMessage &msg) noexcept { msg.order_id = val; };
        handlers[17] = [](std::string_view val, FIXMessage &msg) noexcept { msg.exec_id = val; };
        //clang-format on
        return handlers;
    }
    
    static inline std::array<TagHandlerFunc, MAX_TAG> tagHandlers = makeTagHandlersLookup();

    static inline uint8_t parseNumber(const char *str, size_t len) noexcept
    {
        uint8_t result = 0;
        for (uint8_t i = 0; i < len; ++i)
        {
            char c = str[i];
            result = result * 10 + (c - '0');
        }
        return result;
    }

public:
    FIXMessage parse(const char *data) noexcept
    {
        FIXMessage fixMsg_;
        size_t pos = 0;
        size_t len = strlen(data);

        const __m256i eq_mask = _mm256_set1_epi8('=');
        const __m256i soh_mask = _mm256_set1_epi8('\x01');

        while (pos + 32 <= len) 
        {
            __m256i chunk = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(data + pos));
            __m256i eq_cmp = _mm256_cmpeq_epi8(chunk, eq_mask);
            __m256i soh_cmp = _mm256_cmpeq_epi8(chunk, soh_mask);

            uint32_t eq_bits = _mm256_movemask_epi8(eq_cmp);
            uint32_t soh_bits = _mm256_movemask_epi8(soh_cmp);

            while (eq_bits != 0)
            {
                int eq_offset = __builtin_ctz(eq_bits);
                size_t eq_pos = pos + eq_offset;

                size_t soh_pos = static_cast<size_t>(-1);

                if (soh_bits != 0)
                {
                    int soh_offset = __builtin_ctz(soh_bits);
                    soh_pos = pos + soh_offset;
                }
                else
                {
                    size_t search_pos = eq_pos + 1;
                    while (search_pos < len && data[search_pos] != '\x01')
                        ++search_pos;
                    soh_pos = search_pos;
                }

                std::string_view value(data + eq_pos + 1, soh_pos - (eq_pos + 1));

                uint8_t tag = parseNumber(data + pos, eq_pos - pos);
                auto handler = tagHandlers[tag];
                if (handler)
                    handler(value, fixMsg_);
        

                pos = soh_pos + 1;

                eq_bits &= ~(1u << eq_offset);
                soh_bits &= ~(1u << (soh_pos - pos + eq_offset)); // offset update
            }

            pos += 32;
        }
    
        while (pos < len)
        {
            // '=' işaretini bul
            size_t eq_pos = pos;
            while (data[eq_pos] != '=')
                ++eq_pos;
            // Tag kısmı [pos, eq_pos)
            uint8_t tag = parseNumber(data + pos, eq_pos - pos);
            // SOH karakterini bul
            size_t soh_pos = eq_pos + 1;
            while (data[soh_pos] != '\x01')
                ++soh_pos;

            // Value kısmı [eq_pos+1, soh_pos)
            std::string_view value(data + eq_pos + 1, soh_pos - (eq_pos + 1));

            auto handler = tagHandlers[tag];
            if (handler)
                handler(value, fixMsg_);
    

            pos = soh_pos + 1;
        }
            return fixMsg_;
    }
};
