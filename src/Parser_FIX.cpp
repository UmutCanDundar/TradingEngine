#include "Parser_FIX.h"

Parser_FIX::Parser_FIX(Session_FIX &session, spscFIXInSessionQueue_t &parser_to_fixbuilder_in) noexcept : session_(session), parser_to_fixbuilder_in_(parser_to_fixbuilder_in)
{
   for (size_t i = 0; i < FIX_QUEUE_CAPACITY; i++)
      free_fixMsg_list_.push(&fixMsg_pool_[i]);

   for (size_t i = 0; i < FIX_QUEUE_CAPACITY; i++)
      free_fixSesMsg_list_.push(&fixSesMsg_pool_[i]);
}


std::array<Parser_FIX::TagHandlerFunc, Parser_FIX::MAX_TAG> Parser_FIX::makeTagHandlersLookup() noexcept
{
   std::array<TagHandlerFunc, MAX_TAG> handlers{};
   // clang-format off
        // HEADER
        handlers[35] = [](std::string_view val, FIXMessage *msg) noexcept { msg->msg_type = parseNumber(val.data(), val.size()); };
        handlers[34] = [](std::string_view val, FIXMessage *msg) noexcept { msg->seqnum = static_cast<uint32_t>(parseFixedPoint(val)); };       
        // APPLICATION MESSAGES
        handlers[37] = [](std::string_view val, FIXMessage *msg) noexcept { msg->order_id = val; };
        handlers[11] = [](std::string_view val, FIXMessage *msg) noexcept { msg->cl_ord_id = val; };
        handlers[41] = [](std::string_view val, FIXMessage *msg) noexcept { msg->orig_cl_ord_id = val; };
        handlers[17] = [](std::string_view val, FIXMessage *msg) noexcept { msg->exec_id = val; };
        handlers[150] = [](std::string_view val, FIXMessage *msg) noexcept { msg->exec_type = parseNumber(val.data(), val.size()); };
        handlers[39] = [](std::string_view val, FIXMessage *msg) noexcept { msg->ord_status = parseNumber(val.data(), val.size()); };
        handlers[55] = [](std::string_view val, FIXMessage *msg) noexcept { msg->symbol = val; };
        handlers[48] = [](std::string_view val, FIXMessage *msg) noexcept { msg->instrument_id = static_cast<uint32_t>(parseFixedPoint(val)); };
        handlers[54] = [](std::string_view val, FIXMessage *msg) noexcept { msg->side = static_cast<uint8_t>(parseFixedPoint(val)); };
        handlers[38] = [](std::string_view val, FIXMessage *msg) noexcept { msg->quantity = static_cast<uint32_t>(parseFixedPoint(val)); };
        handlers[40] = [](std::string_view val, FIXMessage *msg) noexcept { msg->ord_type = parseNumber(val.data(), val.size()); };
        handlers[44] = [](std::string_view val, FIXMessage *msg) noexcept { msg->price = parseFixedPoint(val); };
        handlers[32] = [](std::string_view val, FIXMessage *msg) noexcept { msg->last_qty = static_cast<uint32_t>(parseFixedPoint(val)); };
        handlers[31] = [](std::string_view val, FIXMessage *msg) noexcept { msg->last_price = static_cast<uint32_t>(parseFixedPoint(val)); };
        handlers[59] = [](std::string_view val, FIXMessage *msg) noexcept { msg->time_in_force = parseNumber(val.data(), val.size()); };
        handlers[151] = [](std::string_view val, FIXMessage *msg) noexcept { msg->leaves_qty = static_cast<uint32_t>(parseFixedPoint(val)); };
        handlers[14] = [](std::string_view val, FIXMessage *msg) noexcept { msg->filled_qty = static_cast<uint32_t>(parseFixedPoint(val)); };
        handlers[60] = [](std::string_view val, FIXMessage *msg) noexcept { msg->transact_time = static_cast<uint32_t>(parseFixedPoint(val)); };
        handlers[434] = [](std::string_view val, FIXMessage *msg) noexcept { msg->cxl_rej_response_to = static_cast<uint8_t>(parseFixedPoint(val)); };
        handlers[102] = [](std::string_view val, FIXMessage *msg) noexcept { msg->cxl_rej_reason = static_cast<uint8_t>(parseFixedPoint(val)); };

        //clang-format on
        return handlers;
    }
std::array<Parser_FIX::TagHandlerFunc, Parser_FIX::MAX_TAG> Parser_FIX::tagHandlers = makeTagHandlersLookup();


std::array<Parser_FIX::SesTagHandlerFunc, Parser_FIX::MAX_SESTAG> Parser_FIX::makeSesTagHandlersLookup() noexcept
{
   std::array<SesTagHandlerFunc, MAX_SESTAG> handlers{};
   // clang-format off
        // HEADER
        handlers[35] = [](std::string_view val, FIXSessionMessage *msg) noexcept { msg->msg_type = parseNumber(val.data(), val.size()); };
        handlers[34] = [](std::string_view val, FIXSessionMessage *msg) noexcept { msg->seqnum = static_cast<uint32_t>(parseFixedPoint(val)); };
        handlers[43] = [](std::string_view val, FIXSessionMessage *msg) noexcept { msg->possDubFlag = static_cast<uint8_t>(parseNumber(val.data(), val.size())); };
        handlers[122] = [](std::string_view val, FIXSessionMessage *msg) noexcept { msg->org_sending_time = static_cast<uint32_t>(parseFixedPoint(val)); };
        handlers[52] = [](std::string_view val, FIXSessionMessage *msg) noexcept { msg->sending_time = static_cast<uint32_t>(parseFixedPoint(val)); };
        // SESSION MESSAGES
        handlers[141] = [](std::string_view val, FIXSessionMessage *msg) noexcept { msg->reset_seqnum_flag = parseNumber(val.data(), val.size()); };
        handlers[108] = [](std::string_view val, FIXSessionMessage *msg) noexcept { msg->interval = static_cast<uint32_t>(parseFixedPoint(val)); };
        handlers[1409] = [](std::string_view val, FIXSessionMessage *msg) noexcept { msg->ses_status = static_cast<uint32_t>(parseFixedPoint(val)); };
        handlers[123] = [](std::string_view val, FIXSessionMessage *msg) noexcept { msg->gap_fill_flag = parseNumber(val.data(), val.size()); };
        handlers[36] = [](std::string_view val, FIXSessionMessage *msg) noexcept { msg->new_seqnum = static_cast<uint32_t>(parseFixedPoint(val)); };
        handlers[7] = [](std::string_view val, FIXSessionMessage *msg) noexcept { msg->begin_seqnum = static_cast<uint32_t>(parseFixedPoint(val)); };
        handlers[16] = [](std::string_view val, FIXSessionMessage *msg) noexcept { msg->end_seqnum = static_cast<uint32_t>(parseFixedPoint(val)); };
        handlers[45] = [](std::string_view val, FIXSessionMessage *msg) noexcept { msg->ref_seqnum = static_cast<uint32_t>(parseFixedPoint(val)); };
        handlers[373] = [](std::string_view val, FIXSessionMessage *msg) noexcept { msg->reject_reason = static_cast<uint8_t>(parseFixedPoint(val)); };
        handlers[112] = [](std::string_view val, FIXSessionMessage *msg) noexcept { msg->test_req_id = static_cast<uint32_t>(parseFixedPoint(val)); };
        handlers[58] = [](std::string_view val, FIXSessionMessage *msg) noexcept { msg->text = val; };
   //clang-format on
        return handlers;
    }
std::array<Parser_FIX::SesTagHandlerFunc, Parser_FIX::MAX_SESTAG> Parser_FIX::SestagHandlers = makeSesTagHandlersLookup();


