#include "Parser_FIX.h"

#include "SessionManager.h"
#include "NetworkPackets.h"

#include <chrono>
#include <cstring>

Parser_FIX::Parser_FIX(spscFIXInSessionQueue_t &parser_to_fixbuilder_in) noexcept : parser_to_fixbuilder_in_(parser_to_fixbuilder_in)
{
  for (size_t i = 0; i < FIX_QUEUE_CAPACITY; i++) {
    auto* msg = &fixMsg_pool_[i];
    free_fixMsg_list_.push(msg);
}

for (size_t i = 0; i < FIX_QUEUE_CAPACITY; i++) {
    auto* sesMsg = &fixSesMsg_pool_[i];
    free_fixSesMsg_list_.push(sesMsg);
}
}

const std::array<Parser_FIX::TagHandlerFunc, Parser_FIX::MAX_TAG> Parser_FIX::makeTagHandlersLookup() noexcept
{
   constexpr auto ID_SIZE = FIXMessage::ID_SIZE;

   static std::array<TagHandlerFunc, MAX_TAG> handlers{};
// clang-format off
   // HEADER
   handlers[35] = [](std::string_view val, FIXMessage *msg) noexcept { msg->msg_type = parseChar(val); };
   handlers[34] = [](std::string_view val, FIXMessage *msg) noexcept { msg->seqnum = static_cast<uint32_t>(parseNum(val)); };
   // APPLICATION MESSAGES
   handlers[37] = [](std::string_view val, FIXMessage *msg) noexcept { const size_t val_len = std::min(val.size(), ID_SIZE - 1); std::memcpy(msg->order_id, val.data(), val_len); msg->order_id[val_len] = '\0'; };
   handlers[11] = [](std::string_view val, FIXMessage *msg) noexcept { const size_t val_len = std::min(val.size(), ID_SIZE - 1); std::memcpy(msg->cl_ord_id, val.data(), val_len);  msg->cl_ord_id[val_len] = '\0';};
   handlers[41] = [](std::string_view val, FIXMessage *msg) noexcept { const size_t val_len = std::min(val.size(), ID_SIZE - 1); std::memcpy(msg->orig_cl_ord_id, val.data(), val_len); msg->orig_cl_ord_id[val_len] = '\0'; };
   handlers[17] = [](std::string_view val, FIXMessage *msg) noexcept { const size_t val_len = std::min(val.size(), ID_SIZE - 1); std::memcpy(msg->exec_id, val.data(), val_len); msg->exec_id[val_len] = '\0';};
   handlers[150] = [](std::string_view val, FIXMessage *msg) noexcept { msg->exec_type = parseChar(val); };
   handlers[39] = [](std::string_view val, FIXMessage *msg) noexcept { msg->ord_status = parseChar(val); };
   handlers[55] = [](std::string_view val, FIXMessage *msg) noexcept { const size_t val_len = std::min(val.size(), ID_SIZE - 1); std::memcpy(msg->symbol, val.data(), val_len); msg->symbol[val_len] = '\0'; };
   handlers[48] = [](std::string_view val, FIXMessage *msg) noexcept { msg->instrument_id = static_cast<uint32_t>(parseNum(val)); };
   handlers[54] = [](std::string_view val, FIXMessage *msg) noexcept { msg->side = static_cast<uint8_t>(parseNum(val)); };
   handlers[38] = [](std::string_view val, FIXMessage *msg) noexcept { msg->quantity = static_cast<uint32_t>(parseNum(val)); };
   handlers[40] = [](std::string_view val, FIXMessage *msg) noexcept { msg->ord_type = parseChar(val); };
   handlers[44] = [](std::string_view val, FIXMessage *msg) noexcept { msg->price = static_cast<int64_t>(parsePrice<4>(val)); };
   handlers[32] = [](std::string_view val, FIXMessage *msg) noexcept { msg->last_qty = static_cast<uint32_t>(parseNum(val)); };
   handlers[31] = [](std::string_view val, FIXMessage *msg) noexcept { msg->last_price = static_cast<int64_t>(parsePrice<4>(val)); };
   handlers[59] = [](std::string_view val, FIXMessage *msg) noexcept { msg->time_in_force = parseChar(val); };
   handlers[151] = [](std::string_view val, FIXMessage *msg) noexcept { msg->leaves_qty = static_cast<uint32_t>(parseNum(val)); };
   handlers[14] = [](std::string_view val, FIXMessage *msg) noexcept { msg->filled_qty = static_cast<uint32_t>(parseNum(val)); };
   handlers[60] = [](std::string_view val, FIXMessage *msg) noexcept { msg->transact_time = static_cast<uint32_t>(parseNum(val)); };
   handlers[434] = [](std::string_view val, FIXMessage *msg) noexcept { msg->cxl_rej_response_to = static_cast<uint8_t>(parseNum(val)); };
   handlers[102] = [](std::string_view val, FIXMessage *msg) noexcept { msg->cxl_rej_reason = static_cast<uint8_t>(parseNum(val)); };
//clang-format on
   return handlers;
}
const std::array<Parser_FIX::TagHandlerFunc, Parser_FIX::MAX_TAG> Parser_FIX::tagHandlers = makeTagHandlersLookup();


const std::array<Parser_FIX::SesTagHandlerFunc, Parser_FIX::MAX_SESTAG> Parser_FIX::makeSesTagHandlersLookup() noexcept
{
   static std::array<SesTagHandlerFunc, MAX_SESTAG> handlers{};
// clang-format off
   // HEADER
   handlers[35] = [](std::string_view val, FIXSessionMessage *msg) noexcept { msg->msg_type = parseChar(val); };
   handlers[34] = [](std::string_view val, FIXSessionMessage *msg) noexcept { msg->seqnum = static_cast<uint32_t>(parseNum(val)); };
   handlers[43] = [](std::string_view val, FIXSessionMessage *msg) noexcept { msg->possDubFlag = parseChar(val); };
   handlers[122] = [](std::string_view val, FIXSessionMessage *msg) noexcept { msg->org_sending_time = static_cast<uint32_t>(parseNum(val)); };
   handlers[52] = [](std::string_view val, FIXSessionMessage *msg) noexcept { msg->sending_time = static_cast<uint32_t>(parseNum(val)); };
   // SESSION MESSAGES
   handlers[141] = [](std::string_view val, FIXSessionMessage *msg) noexcept { msg->reset_seqnum_flag = parseChar(val); };
   handlers[108] = [](std::string_view val, FIXSessionMessage *msg) noexcept { msg->interval = static_cast<uint32_t>(parseNum(val)); };
   handlers[1409] = [](std::string_view val, FIXSessionMessage *msg) noexcept { msg->ses_status = static_cast<uint32_t>(parseNum(val)); };
   handlers[123] = [](std::string_view val, FIXSessionMessage *msg) noexcept { msg->gap_fill_flag = parseChar(val); };
   handlers[36] = [](std::string_view val, FIXSessionMessage *msg) noexcept { msg->new_seqnum = static_cast<uint32_t>(parseNum(val)); };
   handlers[7] = [](std::string_view val, FIXSessionMessage *msg) noexcept { msg->begin_seqnum = static_cast<uint32_t>(parseNum(val)); };
   handlers[16] = [](std::string_view val, FIXSessionMessage *msg) noexcept { msg->end_seqnum = static_cast<uint32_t>(parseNum(val)); };
   handlers[45] = [](std::string_view val, FIXSessionMessage *msg) noexcept { msg->ref_seqnum = static_cast<uint32_t>(parseNum(val)); };
   handlers[373] = [](std::string_view val, FIXSessionMessage *msg) noexcept { msg->reject_reason = static_cast<uint8_t>(parseNum(val)); };
   handlers[112] = [](std::string_view val, FIXSessionMessage *msg) noexcept { msg->test_req_id = static_cast<uint32_t>(parseNum(val)); };
   handlers[58] = [](std::string_view val, FIXSessionMessage *msg) noexcept { const size_t val_len = std::min(val.size(), FIXSessionMessage::TEXT_SIZE - 1); std::memcpy(msg->text, val.data(), val_len); msg->text[val_len] = '\0'; };
//clang-format on
   return handlers;
}
const std::array<Parser_FIX::SesTagHandlerFunc, Parser_FIX::MAX_SESTAG> Parser_FIX::SestagHandlers = makeSesTagHandlersLookup();

bool Parser_FIX::handle_sesMsg(FIXSessionMessage *fixSesMsg, Sequence_FIX& seq_fix, SessionState& state) noexcept
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
               if(fixSesMsg->ses_status == 255)
                  return false;
               else if (fixSesMsg->ses_status == 4)
                  state.is_logged_in.store(false,std::memory_order_release);


               return true;

            case FIXTypes::Logon:
               if(fixSesMsg->reset_seqnum_flag == 'Y')
               {
                  seq_fix.set_expected_seq(2);
                  seq_fix.set_next_seq(1);
               }
               state.is_logged_in.store(true, std::memory_order_release);
               state.logged_before.store(true, std::memory_order_release);
               state.login_retry_count = 0;

               return true;

            default:
               return true;

      }
}

char Parser_FIX::find_type(const char* data) noexcept
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

void Parser_FIX::resend_logic(uint32_t msg_seqnum, Sequence_FIX &seq_fix) noexcept
{
   auto now_ts = std::chrono::steady_clock::now();
   auto last_resend_ts = seq_fix.get_last_resend_ts();

   if (seq_fix.get_resend_counter() < FIXSequence::MAX_RESEND_ATTEMPT &&
      now_ts - last_resend_ts > FIXSequence::RESEND_INTERVAL)
   {
      FIXSessionMessage *msg_1 = nullptr;
      free_fixSesMsg_list_.pop(msg_1);
      msg_1->begin_seqnum = seq_fix.get_expected();
      msg_1->end_seqnum = msg_seqnum;
      msg_1->msg_type = static_cast<uint8_t>(FIXTypes::ResendRequest);
      parser_to_fixbuilder_in_.push(msg_1);

      seq_fix.increase_resend_counter();
   }
   else
   {
      FIXSessionMessage *msg_2 = nullptr;
      free_fixSesMsg_list_.pop(msg_2);
      msg_2->msg_type = static_cast<uint8_t>(FIXTypes::Logout);
      parser_to_fixbuilder_in_.push(msg_2);

      FIXSessionMessage *msg_3 = nullptr;
      free_fixSesMsg_list_.pop(msg_3);
      msg_3->msg_type = static_cast<uint8_t>(FIXTypes::Logon);
      parser_to_fixbuilder_in_.push(msg_3);

      // pending_to_store_map_.clear();
      seq_fix.reset_resend_counter();
   }
}

void Parser_FIX::resend_logic_logon(FIXSessionMessage *fixSesMsg, Sequence_FIX &seq_fix) noexcept
{
   if (fixSesMsg->reset_seqnum_flag == 'Y')
   {
      seq_fix.set_expected_seq(2);
      seq_fix.set_next_seq(1);
      pending_to_store_map_.clear();

      releaseFIX(fixSesMsg);
      return;
   }

   FIXSessionMessage *msg = nullptr;
   free_fixSesMsg_list_.pop(msg);
   msg->begin_seqnum = seq_fix.get_expected();
   msg->end_seqnum = fixSesMsg->seqnum - 1;
   msg->msg_type = static_cast<uint8_t>(FIXTypes::ResendRequest);
   parser_to_fixbuilder_in_.push(msg);
   push_pending(fixSesMsg);
}

std::pair<char*, size_t> Parser_FIX::nextFixMsg(RxPacket *pkt, size_t& data_offset) noexcept
{
   if (data_offset >= pkt->len)
      return {nullptr, 0};
   
   auto* partial_data = partial_fix_msg_.data;
   
   // Partial Packet Handler, Start
   if (partial_fix_msg_.active == true && pkt->consumed == false)
   {
      size_t &existing_len = partial_fix_msg_.existing_len;
      size_t &remaining_len = partial_fix_msg_.remaining_len;

      if(remaining_len == 0 && partial_fix_msg_.bodylen_state != BodyLenState::DONE)
      {
         size_t pos = data_offset;
         size_t body_len = 0;
         size_t body_len_end = 0;

         switch (partial_fix_msg_.bodylen_state)
         {
            case BodyLenState::EXPECT_SOH:
               while (pos < pkt->len && pkt->data[pos] != '\x01')
                     ++pos;
               pos += 3;
               break;
            case BodyLenState::EXPECT_9:
               while (pos < pkt->len && pkt->data[pos] != '9')
                     ++pos;
               pos += 2;
               break;
            case BodyLenState::EXPECT_EQ:
               while (pos < pkt->len && pkt->data[pos] != '=')
                     ++pos;
               pos += 1;
               break;
            case BodyLenState::READ_VALUE:
               if (pos < pkt->len && pkt->data[pos] == '\x01')
                  body_len = partial_fix_msg_.body_len;
               break;
            default:
               break;
         }

         if(partial_fix_msg_.bodylen_state != BodyLenState::DONE)
         {
              
            if (pos < pkt->len && pkt->data[pos] >= '0' && pkt->data[pos] <= '9')
            {
               while (pos < pkt->len && pkt->data[pos] != '\x01')
               {
                  partial_fix_msg_.body_len *= 10; 
                  partial_fix_msg_.body_len += (pkt->data[pos] - '0');
                  ++pos;
               }

               body_len = partial_fix_msg_.body_len;
            }
            
            if (pos >= pkt->len)
            {
               partial_fix_msg_.bodylen_state = pkt->data[pos] == '\x01' ? BodyLenState::DONE : BodyLenState::READ_VALUE;
               body_len = partial_fix_msg_.body_len;
            }
            
            if(partial_fix_msg_.bodylen_state == BodyLenState::DONE)
            {
               body_len_end = pos + 1;
               remaining_len = body_len_end + body_len + 7;
            }
            else
            {
               partial_fix_msg_.bodylen_state = BodyLenState::READ_VALUE;
               std::memcpy(partial_data + existing_len, pkt->data.data() + data_offset, pos - data_offset);
               pkt->consumed = true;

               return {nullptr, 0};
            }
         }
      }

      size_t copy_len = std::min(remaining_len, static_cast<size_t>(pkt->len));
      std::memcpy(partial_data + existing_len, pkt->data.data() + data_offset, copy_len);

      existing_len += copy_len;
      remaining_len -= copy_len;
      pkt->consumed = true;

      if(copy_len < pkt->len)
      {
         pkt->consumed = false;
         data_offset += copy_len;
      }

      if(remaining_len == 0)
      {
            const size_t msg_len = existing_len;
            existing_len = 0;
            partial_fix_msg_.active = false;
            partial_fix_msg_.body_len = 0;
            partial_fix_msg_.bodylen_state = BodyLenState::None;

            return {partial_data, msg_len};
      }
      else
      {
            return{nullptr, 0};
      }
   }
   //Partial Packet Handler, End

   // Finding BodyLen Of The Message, Start
   size_t next_msg_start = data_offset;
   size_t body_len = 0;
   size_t body_len_end = 0;

   for (size_t msg_offset = 0; next_msg_start + msg_offset < pkt->len; ++msg_offset)
   {
      if (pkt->data[next_msg_start + msg_offset] == '\x01')
      {
            partial_fix_msg_.bodylen_state = BodyLenState::EXPECT_9;
            continue;
      }

      if (pkt->data[next_msg_start + msg_offset] == '9')
      {
            partial_fix_msg_.bodylen_state = BodyLenState::EXPECT_EQ;
            continue;
      }

      if (pkt->data[next_msg_start + msg_offset] == '=')
      {
            partial_fix_msg_.bodylen_state = BodyLenState::READ_VALUE;

            size_t pos = next_msg_start + msg_offset + 1;
            while (pos < pkt->len && pkt->data[pos] >= '0' && pkt->data[pos] <= '9')
            {
               body_len = body_len * 10 + (pkt->data[pos] - '0');
               ++pos;

               if (pos == pkt->len)
                  partial_fix_msg_.body_len = body_len;
            }

            if (pos < pkt->len && pkt->data[pos] == '\x01')
            {
               partial_fix_msg_.bodylen_state = BodyLenState::DONE;
               body_len_end = pos + 1;
            }
      }

      if(partial_fix_msg_.bodylen_state == BodyLenState::DONE)
         break;
   }
   // Finding BodyLen Of The Message, End

   // Returns The Message And Its Len (If It Is Not Partial)
   size_t checksum_start = body_len_end + body_len;
   size_t msg_len = 0;
   size_t existing_len = pkt->len - data_offset; // could be the length of a partial message or whole message.

   if (body_len_end == 0)
   {
      std::memcpy(partial_data, pkt->data.data() + data_offset, existing_len);
      partial_fix_msg_.existing_len = existing_len;
      partial_fix_msg_.active = true;
      pkt->consumed = true;

      if(body_len == 0)
         partial_fix_msg_.bodylen_state = BodyLenState::EXPECT_SOH;

      return {nullptr, 0};
   }
   else
   {
      msg_len = checksum_start + 7 - next_msg_start;

      if(msg_len > existing_len)
      {
         std::memcpy(partial_data, pkt->data.data() + data_offset, existing_len);
         pkt->consumed = true;
         data_offset += existing_len;
         partial_fix_msg_.remaining_len = msg_len - existing_len;
         partial_fix_msg_.active = true;
         partial_fix_msg_.existing_len = existing_len;
         partial_fix_msg_.bodylen_state = BodyLenState::DONE;

         return {nullptr, 0};
      }
      else if(msg_len < existing_len)
      {

         std::memcpy(partial_data, pkt->data.data() + data_offset, msg_len);
         pkt->consumed = false;
         data_offset += msg_len;
         partial_fix_msg_.remaining_len = 0;
         partial_fix_msg_.active = false;
         partial_fix_msg_.body_len = 0;
         partial_fix_msg_.existing_len = existing_len - msg_len;
         partial_fix_msg_.bodylen_state = BodyLenState::None;
         
         return {partial_data, msg_len};
      }
      else
      {
         pkt->consumed = true;
         partial_fix_msg_.active = false;
         partial_fix_msg_.body_len = 0;
         partial_fix_msg_.bodylen_state = BodyLenState::None;
         return {pkt->data.data() + data_offset, msg_len};
      }

   }
}