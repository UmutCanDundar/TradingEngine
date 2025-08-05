#include "Parser_FIX.h"

Parser_FIX::Parser_FIX() noexcept
{
   fixMsg_pool_.set_next_size(FIX_QUEUE_CAPACITY);
   for (size_t i = 0; i <= FIX_QUEUE_CAPACITY; i++)
      free_fixMsg_list_.push(fixMsg_pool_.construct());
}

std::array<Parser_FIX::TagHandlerFunc, Parser_FIX::MAX_TAG> Parser_FIX::makeTagHandlersLookup() noexcept
{
   std::array<TagHandlerFunc, MAX_TAG> handlers{};
   // clang-format off
        handlers[8]  = [](std::string_view val, FIXMessage *msg) noexcept { msg->fix_version = val; };
        handlers[35] = [](std::string_view val, FIXMessage *msg) noexcept { msg->msg_type = val[0]; };
        handlers[44] = [](std::string_view val, FIXMessage *msg) noexcept { msg->price = parseFixedPoint(val); };
        handlers[38] = [](std::string_view val, FIXMessage *msg) noexcept { msg->quantity = static_cast<uint32_t>(parseFixedPoint(val)); };
        handlers[151] = [](std::string_view val, FIXMessage *msg) noexcept { msg->leaves_qty = static_cast<uint32_t>(parseFixedPoint(val)); };
        handlers[32] = [](std::string_view val, FIXMessage *msg) noexcept { msg->filled_qty = static_cast<uint32_t>(parseFixedPoint(val)); };
        handlers[60] = [](std::string_view val, FIXMessage *msg) noexcept { msg->transact_time = static_cast<uint32_t>(parseFixedPoint(val)); };
        handlers[54] = [](std::string_view val, FIXMessage *msg) noexcept { msg->side = val[0]; };
        handlers[40] = [](std::string_view val, FIXMessage *msg) noexcept { msg->ord_type = val[0]; };
        handlers[59] = [](std::string_view val, FIXMessage *msg) noexcept { msg->time_in_force = val[0]; };
        handlers[39] = [](std::string_view val, FIXMessage *msg) noexcept { msg->ord_status = val[0]; };
        handlers[150] = [](std::string_view val, FIXMessage *msg) noexcept { msg->exec_type = val[0]; };
        handlers[55] = [](std::string_view val, FIXMessage *msg) noexcept { msg->symbol = val; };
        handlers[11] = [](std::string_view val, FIXMessage *msg) noexcept { msg->cl_ord_id = val; };
        handlers[37] = [](std::string_view val, FIXMessage *msg) noexcept { msg->order_id = val; };
        handlers[17] = [](std::string_view val, FIXMessage *msg) noexcept { msg->exec_id = val; };
        //clang-format on
        return handlers;
    }

FIXMessage* Parser_FIX::parse(const char *data) noexcept
{
   FIXMessage* fixMsg_ {nullptr};
   free_fixMsg_list_.pop(fixMsg_);

   size_t pos = 0;
   size_t len = strlen(data);

   const __m256i eq_mask = _mm256_set1_epi8('=');
   const __m256i soh_mask = _mm256_set1_epi8('\x01');

   while (pos + 32 <= len)
   {
      __m256i chunk = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(data + pos));
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

std::array<Parser_FIX::TagHandlerFunc, Parser_FIX::MAX_TAG> Parser_FIX::tagHandlers = makeTagHandlersLookup();