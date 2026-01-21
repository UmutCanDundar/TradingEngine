#pragma once

#include "common.h"
#include "Sequence_FIX.h"
#include "Logger.h"
#include "NetworkIO.h"

#include <string_view>
#include <cstdint>
#include <array>
#include <cstring>
#include <cstdio>
#include <vector>
#include <immintrin.h>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/pool/object_pool.hpp>
#include <variant>
#include <absl/container/btree_map.h>
#include <type_traits>
#include <chrono>
#include <algorithm>

enum class BodyLenState : uint8_t
{
    EXPECT_SOH,
    EXPECT_9,  
    EXPECT_EQ, 
    READ_VALUE, 
    DONE,
    None,
};

struct HalfFIXBuffer 
{
    static constexpr size_t DATA_SIZE = 1024;
    char data[DATA_SIZE];
    size_t existing_len = 0;
    size_t remaining_len = 0;
    size_t body_len = 0;
    bool active = false;
    BodyLenState bodylen_state = BodyLenState::None;
};
    
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
};

/* enum class LogonStatus : uint8_t
{
    Active = 0,
    Password_changed = 1,
    Password_error = 3,
    Invalid_id = 9,
    Invalid_bodylength = 100, // 100 = crtical error-session suspended --- requires manual intervention
    Low_interval = 101,
    Unknown = 255
}; */

enum class LogoutStatus : uint8_t
{
    Password_Error = 3,
    Logged_out = 4,
    Invalid_userinfo = 5,
    Account_locked = 6,
    Password_expired = 8,
    Invalid_id = 9,
    Invalid_bodylength = 100, // 100 = crtical error-session suspended --- requires manual intervention
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
    Incorrect_data_format_for_value = 6, // 100 = crtical error-session suspended --- requires manual intervention
    Unknown = 255
};

struct alignas(64) FIXMessage
{
    int64_t price = 0;         // 8 byte, FIX Tag 44: Fiyat (fixed-point encoding)
    int64_t last_price = 0;

    uint32_t quantity = 0;      // 4 byte, FIX Tag 38: Miktar (Order Qty)
    uint32_t leaves_qty = 0;    // 4 byte, FIX Tag 151: Kalan miktar (Remaining Qty)
    uint32_t last_qty = 0;      // 4 byte, FIX Tag 14: Doldurulan miktar (Exec Qty)
    uint32_t filled_qty = 0;    // 4 byte, FIX Tag 32: Doldurulan miktar (Filled Qty (cum))
    uint32_t transact_time = 0; // 4 byte, FIX Tag 60: İşlem zamanı (UTC timestamp saniye cinsinden)
    uint32_t instrument_id = 0;
    uint32_t seqnum = 0;

    uint8_t msg_type = 0;      // 1 byte, FIX Tag 35: Mesaj tipi (NewOrder, ExecReport, vb.)
    uint8_t side = 0;          // 1 byte, FIX Tag 54: Emir yönü (Buy/Sell)
    uint8_t ord_status = 0;    // 1 byte, FIX Tag 39: Emir durumu (New, Filled, Cancelled, vb.)
    uint8_t exec_type = 0;     // 1 byte, FIX Tag 150: Gerçekleşme tipi (Trade, Cancel, vb.)
    uint8_t ord_type = 0;      // 1 byte, FIX Tag 40: Emir tipi (Limit, Market, vb.)
    uint8_t time_in_force = 0; // 1 byte, FIX Tag 59: Geçerlilik süresi
    uint8_t cxl_rej_response_to = 0;
    uint8_t cxl_rej_reason = 255;

    uint8_t pad1[12];

    //cache-line
    std::string_view symbol; // 16 byte, FIX Tag 55: İşlem gören varlık (Symbol) 
    std::string_view order_id;  // 16 byte, FIX Tag 37: Broker tarafından atanan emir ID'si (Order ID)
    std::string_view cl_ord_id; // 16 byte, FIX Tag 11: Müşteri emir ID'si (Client Order ID)
    std::string_view exec_id;   // 16 byte, FIX Tag 17: Gerçekleşme ID'si (Execution ID)
    
    // cache-line
    std::string_view orig_cl_ord_id;
    uint8_t pad2[48];
};

struct alignas(64) FIXSessionMessage
{
    uint32_t seqnum = 0;
    uint32_t interval = 0;      
    uint32_t ses_status = 255;
    uint32_t new_seqnum = 0;    
    uint32_t begin_seqnum = 0; 
    uint32_t end_seqnum = 0;     
    uint32_t ref_seqnum = 0;         
    uint32_t ord_status = 0; 
    uint32_t test_req_id = 0;
    uint32_t sending_time = 0;
    uint32_t org_sending_time = 0;

    uint8_t gap_fill_flag = 0;      
    uint8_t reject_reason = 255;     
    uint8_t msg_type = 0;
    uint8_t possDubFlag = 0;

    std::string_view text;

    //cache-line
    uint8_t reset_seqnum_flag = 'N';
   
    uint8_t pad2[63];
};

inline constexpr size_t FIX_QUEUE_CAPACITY = 1024;
inline constexpr size_t MESSAGE_QUEUE_CAPACITY = 1024;

using FIXMessagePool = std::array<FIXMessage, FIX_QUEUE_CAPACITY>;
using spscFIXQueue_t = boost::lockfree::spsc_queue<FIXMessage*, boost::lockfree::capacity<FIX_QUEUE_CAPACITY>>;

using FIXSesMessagePool = std::array<FIXSessionMessage, FIX_QUEUE_CAPACITY>;
using spscFIXSesQueue_t = boost::lockfree::spsc_queue<FIXSessionMessage*, boost::lockfree::capacity<FIX_QUEUE_CAPACITY>>;

using spscFIXInSessionQueue_t = boost::lockfree::spsc_queue<FIXSessionMessage*, boost::lockfree::capacity<MESSAGE_QUEUE_CAPACITY>>;

using FIXPendingMap_t = absl::btree_map<uint32_t, std::variant<FIXMessage*, FIXSessionMessage*>>;

class Parser_FIX
{
private:
    HalfFIXBuffer partial_fix_msg;

    FIXMessagePool fixMsg_pool_;
    spscFIXQueue_t free_fixMsg_list_;

    FIXSesMessagePool fixSesMsg_pool_;
    spscFIXSesQueue_t free_fixSesMsg_list_;
    
    FIXPendingMap_t pending_to_store_map_;

    static constexpr char SOH = '\x01';
    static constexpr size_t MAX_TAG = 512;
    static constexpr size_t MAX_SESTAG = 2048;

    using TagHandlerFunc = void (*)(std::string_view, FIXMessage *) noexcept;
    static const std::array<TagHandlerFunc, MAX_TAG>& makeTagHandlersLookup() noexcept;
    static const std::array<TagHandlerFunc, MAX_TAG>& tagHandlers;

    using SesTagHandlerFunc = void (*)(std::string_view, FIXSessionMessage *) noexcept;
    static const std::array<SesTagHandlerFunc, MAX_SESTAG>& makeSesTagHandlersLookup() noexcept;
    static const std::array<SesTagHandlerFunc, MAX_SESTAG>& SestagHandlers;

    SessionManager &sess_mngr_;
    spscFIXInSessionQueue_t &parser_to_fixbuilder_in_;

public:
    Parser_FIX(SessionManager &sess_mngr, spscFIXInSessionQueue_t &parser_to_fixbuilder_in) noexcept;

    template <typename T>
    T* parse(const char *data, size_t len) noexcept
    {
        T* msg{nullptr};

        if constexpr (std::is_same_v<T, FIXMessage>)
            free_fixMsg_list_.pop(msg);
        else
            free_fixSesMsg_list_.pop(msg);

        auto const &handlers = []() -> auto const &
        {
            if constexpr (std::is_same_v<T, FIXMessage>)
                return tagHandlers;
            else
                return SestagHandlers;
        }();

        size_t pos = 0;
        size_t tag_start = 0;

        const __m256i eq_mask = _mm256_set1_epi8('=');
        const __m256i soh_mask = _mm256_set1_epi8('\x01');

        while (pos + 32 <= len)
        {
            __m256i chunk = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(data + pos));

            uint32_t eq_bits = _mm256_movemask_epi8(_mm256_cmpeq_epi8(chunk, eq_mask));
            uint32_t soh_bits = _mm256_movemask_epi8(_mm256_cmpeq_epi8(chunk, soh_mask));

            while (eq_bits != 0)
            {

                int eq_offset = __builtin_ctz(eq_bits);
                size_t eq_pos = pos + eq_offset;

                int soh_offset = 0;
                size_t soh_pos = static_cast<size_t>(-1);

                uint32_t soh_after = soh_bits & (~0u << (eq_offset + 1));

                if (soh_after != 0)
                {
                    soh_offset = __builtin_ctz(soh_after);
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

                uint32_t tag = parseNumber(data + tag_start, eq_pos - tag_start);

                auto& handler = handlers[tag];
                if (handler)
                    handler(value, msg);

                tag_start = soh_pos + 1;

                eq_bits &= ~(1u << eq_offset);
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
            uint32_t tag = parseNumber(data + tag_start, eq_pos - tag_start);
            // SOH karakterini bul
            size_t soh_pos = eq_pos + 1;
            while (data[soh_pos] != '\x01')
                ++soh_pos;

            // Value kısmı [eq_pos+1, soh_pos)
            std::string_view value(data + eq_pos + 1, soh_pos - (eq_pos + 1));

             auto& handler = handlers[tag];
             if (handler)
                handler(value, msg);

            tag_start = soh_pos + 1;
            pos = soh_pos + 1;
        }
        return msg;
    }

    inline bool handle_sesMsg(FIXSessionMessage *fixSesMsg, Sequence_FIX& seq_fix) noexcept
    {
        const FIXTypes type = static_cast<FIXTypes>(fixSesMsg->msg_type);
      
            switch(type) 
            {
                case FIXTypes::ResendRequest:
                    return false;
                
                case FIXTypes::Reject:
                    return false;
                
                case FIXTypes::SequenceReset:
                    seq_fix.set_expected_seq(fixSesMsg->new_seqnum);
                    return true;
                
                case FIXTypes::TestRequest:
                    return false;
                
                case FIXTypes::Logout:
                    return true;
                
                case FIXTypes::Logon:
                    if(fixSesMsg->reset_seqnum_flag == 'Y') 
                    {
                        seq_fix.set_expected_seq(2);
                        seq_fix.set_next_seq(1);
                    }
                    return true;

                default:
                    return true;

            } 
    }

    inline void releaseFIX(FIXMessage* fixMsg) noexcept
    {
        free_fixMsg_list_.push(fixMsg);
    }
    inline void releaseFIX(FIXSessionMessage* fixSesMsg) noexcept
    {
        free_fixSesMsg_list_.push(fixSesMsg);
    }

    inline char find_type(const char* data)
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

    template<typename T>
    inline void push_pending(T* msg) noexcept
    {
        using type_t = std::conditional_t<std::is_same_v<T, FIXMessage>, FIXMessage*, FIXSessionMessage*>;
        pending_to_store_map_.emplace(
            msg->seqnum,
            std::variant<FIXMessage *, FIXSessionMessage *>{std::in_place_type<type_t>, msg});
    }
    inline std::variant<FIXMessage*, FIXSessionMessage*>* find_in_pending(const uint32_t expected) noexcept
    {
        auto it = pending_to_store_map_.find(expected);
        if (it == pending_to_store_map_.end())
            return nullptr;

        return &it->second;
    }
    inline void pop_pending(const auto seqnum) noexcept
    {
        pending_to_store_map_.erase(seqnum);
    }

    inline void resend_logic(uint32_t msg_seqnum, Sequence_FIX &seq_fix) noexcept
    {
        auto now_ts = std::chrono::steady_clock::now();
        auto last_resend_ts = seq_fix.get_last_resend_ts();

        if (seq_fix.get_resend_counter() < FIXSequence::MAX_RESEND_ATTEMPT && 
            now_ts - last_resend_ts > FIXSequence::RESEND_INTERVAL)
        {
            FIXSessionMessage *msg_1;
            free_fixSesMsg_list_.pop(msg_1);
            msg_1->begin_seqnum = seq_fix.get_expected(); 
            msg_1->end_seqnum = msg_seqnum;
            msg_1->msg_type = static_cast<uint8_t>(FIXTypes::ResendRequest);
            parser_to_fixbuilder_in_.push(msg_1);

            seq_fix.increase_resend_counter(); 
        }
        else 
        {
            FIXSessionMessage *msg_2;
            free_fixSesMsg_list_.pop(msg_2);
            msg_2->msg_type = static_cast<uint8_t>(FIXTypes::Logout);
            parser_to_fixbuilder_in_.push(msg_2);

            FIXSessionMessage *msg_3;
            free_fixSesMsg_list_.pop(msg_3);
            msg_3->msg_type = static_cast<uint8_t>(FIXTypes::Logon);
            parser_to_fixbuilder_in_.push(msg_3);

            pending_to_store_map_.clear();
            seq_fix.reset_resend_counter();
        }
    }
    inline void resend_logic_logon(FIXSessionMessage *fixSesMsg, Sequence_FIX &seq_fix) noexcept
    {
        if (fixSesMsg->reset_seqnum_flag == 'Y')
        {
            seq_fix.set_expected_seq(2);
            seq_fix.set_next_seq(1);
            pending_to_store_map_.clear(); 

            releaseFIX(fixSesMsg);
            return;
        }
        
        FIXSessionMessage *msg;
        free_fixSesMsg_list_.pop(msg);
        msg->begin_seqnum = seq_fix.get_expected();
        msg->end_seqnum = fixSesMsg->seqnum - 1;
        msg->msg_type = static_cast<uint8_t>(FIXTypes::ResendRequest);
        parser_to_fixbuilder_in_.push(msg);
        push_pending(fixSesMsg);
    }

    inline std::pair<char*, size_t> nextFixMsg(OutPacket *pkt, size_t& data_offset) noexcept
    {

        if (data_offset >= pkt->len)
            return {nullptr, 0};
   
        auto *partial_data = partial_fix_msg.data;
        
        // Partial Packet Handler Start
        if (partial_fix_msg.active == true)
        {
            size_t &existing_len = partial_fix_msg.existing_len;
            size_t &remaining_len = partial_fix_msg.remaining_len;

            if(remaining_len == 0)
            {
                size_t pos = data_offset;
                size_t body_len = 0;
                size_t body_len_end = 0;

                switch (partial_fix_msg.bodylen_state)
                {
                    case BodyLenState::EXPECT_SOH:
                        while (pos < pkt->len && pkt->data[pos] != '\x01')
                            ++pos;
                        pos += 3;
                        break;
                    case BodyLenState::EXPECT_9:
                        pos += 2; 
                        break;
                    case BodyLenState::EXPECT_EQ:
                        pos += 1;
                        break;
                    case BodyLenState::READ_VALUE:
                        body_len = partial_fix_msg.body_len;
                        break;
                    default:
                        break;
                }

                while (pos < pkt->len && pkt->data[pos] >= '0' && pkt->data[pos] <= '9')
                {
                    body_len = body_len * 10 + (pkt->data[pos] - '0');
                    ++pos;

                    if (pos == pkt->len)
                    {
                        partial_fix_msg.body_len = body_len;
                        return{nullptr, 0};
                    }
                }                    

                partial_fix_msg.bodylen_state = BodyLenState::DONE;
                body_len_end = pos + 1;
                remaining_len = body_len_end + body_len + 7;
                
            }
              
            size_t copy_len = std::min(remaining_len, static_cast<size_t>(pkt->len));
            std::memcpy(partial_data + existing_len, pkt->data.data() + data_offset, copy_len);

            existing_len += copy_len;
            remaining_len -= copy_len;

            if(remaining_len == 0)
            {
                const size_t msg_len = existing_len;
                existing_len = 0;
                remaining_len = 0;
                partial_fix_msg.active = false;
                partial_fix_msg.bodylen_state = BodyLenState::None;

                return {partial_data, msg_len};
            }
            else
            {
                return{nullptr, 0};
            }
        } 
        //Partial Packet Handler End

        // Finding BodyLen Of The Message Start
        size_t next_msg_start = data_offset;
        size_t body_len = 0;
        size_t body_len_end = 0;

        for (size_t msg_offset = 0; next_msg_start + msg_offset < pkt->len; ++msg_offset)
        {
            if (pkt->data[next_msg_start + msg_offset] == '\x01')
            {
                partial_fix_msg.bodylen_state = BodyLenState::EXPECT_9;
                continue;
            }

            if (pkt->data[next_msg_start + msg_offset] == '9')
            {
                partial_fix_msg.bodylen_state = BodyLenState::EXPECT_EQ;
                continue;
            }

            if (pkt->data[next_msg_start + msg_offset] == '=')
            {
                partial_fix_msg.bodylen_state = BodyLenState::READ_VALUE;

                size_t pos = next_msg_start + msg_offset + 1;
                while (pos < pkt->len && pkt->data[pos] >= '0' && pkt->data[pos] <= '9')
                {
                    body_len = body_len * 10 + (pkt->data[pos] - '0');
                    ++pos;
                    
                    if (pos == pkt->len)
                        partial_fix_msg.body_len = body_len;
                }
               
                if (pos < pkt->len && pkt->data[pos] == '\x01')
                {
                    partial_fix_msg.bodylen_state = BodyLenState::DONE;
                    body_len_end = pos + 1;
                }
            }

            break;
        }
        // Finding BodyLen Of The Message End

        // Returns The Message And Its Len (If It Is Not Partial)
        size_t checksum_start = body_len_end + body_len;
        size_t msg_len = 0;

        if (body_len_end == 0 || (body_len_end > 0 && (checksum_start + 7 > pkt->len)))
        {
            size_t existing_len = pkt->len - data_offset;
            std::memcpy(partial_data, pkt->data.data() + data_offset, existing_len);
            partial_fix_msg.existing_len = existing_len;
            data_offset = pkt->len;
            partial_fix_msg.active = true;

            if(body_len_end > 0)
            {
                msg_len = checksum_start + 7;
                partial_fix_msg.remaining_len = msg_len - existing_len;
                partial_fix_msg.bodylen_state = BodyLenState::DONE;
            }
            else if(body_len == 0)
            {
                partial_fix_msg.bodylen_state = BodyLenState::EXPECT_SOH;
            }
            
            return {nullptr, 0};
        }
        else
        {
            msg_len = checksum_start + 7;
            return {pkt->data.data() + data_offset, msg_len};
        }
    }

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


/* static constexpr std::string_view to_string(SessionRejectReason reason) 
   {
        switch(reason)
        {
            case SessionRejectReason::Invalid_tag_number:
                return "Invalid_tag_number";
            case SessionRejectReason::Required_tag_missing:
                return "Required_tag_missing";
            case SessionRejectReason::Tag_not_defined_for_this_type:
                return "Tag_not_defined_for_this_type";
            case SessionRejectReason::Undefined_tag:
                return "Undefined_tag";
            case SessionRejectReason::Tag_specified_without_a_value:
                return "Tag_specified_without_a_value";
            case SessionRejectReason::Out_of_range_value:
                return "Out_of_range_value";
            case SessionRejectReason::Incorrect_data_format_for_value:
                return "Incorrect_data_format_for_value";
            case SessionRejectReason::CompID_problem:
                return "CompID_problem";
            case SessionRejectReason::Sendingtime_accuracy_problem:
                return "Sendingtime_accuracy_problem";
            case SessionRejectReason::Invalid_type:
                return "Invalid_type";
            case SessionRejectReason::Repeating_group_fields:
                return "Repeating_group_fields";
            case SessionRejectReason::Incorrect_numInGroup:
                return "Incorrect_numInGroup";
            case SessionRejectReason::Other:
                return "Other";
            default:
                return "Unknown";
        }
   }

   static constexpr std::string_view to_string(LogoutStatus status)
   {
        switch (status)
       {
       case LogoutStatus::Password_Error:
           return "Password_Error";
       case LogoutStatus::Logged_out:
           return "Logged_out";
       case LogoutStatus::Invalid_userinfo:
           return "Invalid_userinfo";
       case LogoutStatus::Account_locked:
           return "Account_locked";
       case LogoutStatus::Password_expired:
           return "Password_expired";
       case LogoutStatus::Invalid_id:
           return "Invalid_id";
       case LogoutStatus::Invalid_bodylength:
           return "Invalid_bodylength ";
       case LogoutStatus::Low_interval:
           return "Low_interval";
       default:
           return "Unknown";
       }
   } */