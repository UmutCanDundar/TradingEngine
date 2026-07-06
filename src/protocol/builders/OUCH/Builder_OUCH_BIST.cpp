//FUNC_PTR PRE-FILL

#include "Builder_OUCH_BIST.h"
#include "SoupBinTcp.h"
#include "SessionManager.h"

Builder_OUCH_BIST::Builder_OUCH_BIST(SoupBinTcp &sbt, SessionManager &sess_mngr) noexcept : eci_local_(initialize_exc_info(sess_mngr)), sbt_(sbt) 
{}

Buffer_OBT* Builder_OUCH_BIST::build(const Order &order) noexcept
{
    Buffer_OBT* buffer = nullptr;

    switch(order.message_type)
    {
        case 0:
            buffer = &enter_messages[next_enter++ & (DAILY_ENTER - 1)];
            break;
        case 1:
            buffer = &replace_messages[(next_replace++ & (DAILY_REPLACE - 1))];
            break;
        case 2:
            buffer = &cancel_messages[(next_cancel++ & (DAILY_CANCEL - 1))];
            break;
        case 3:
            buffer = &cancelid_messages[(next_cancelid++ & (DAILY_CANCELID - 1))];
            break;
    }

    char *buf = buffer->msg;
    buf = sbt_.WriteSBTHeaderForDataPacket(buf, buffer->len);

    (this->*OuchMessageBuilders[order.message_type])(order, buf);

    return buffer;
}

std::array<ExchangeClientInfo_local, ACC_COUNT> Builder_OUCH_BIST::initialize_exc_info(SessionManager &sess_mngr) noexcept 
{
    std::array<ExchangeClientInfo_local, ACC_COUNT> eci_local;

    for (size_t acc_index = 0; acc_index < ACC_COUNT; ++acc_index)
    {
        const auto sess_index = sess_mngr.getSessionIndex(Venue::BIST, Protocol::OUCH, acc_index);

        const auto* sess_context = sess_mngr.getSessionContext(sess_index);
        const size_t auth_index = sess_context->tcp_index;
        const auto &eci = sess_mngr.getSessionAuth(auth_index)->sbt.eci;
        eci_local[acc_index] = ExchangeClientInfo_local{std::string_view{eci.client_account.data(), eci.client_account.size()},
                                                        std::string_view{eci.exchange_info.data(), eci.exchange_info.size()}};
    }

    return eci_local;
}







// DISCARDED ALTERNATIVES

// FUNC-PTR NO-PRE-FILL

// #include "Builder_OUCH_BIST.h"
// #include "SoupBinTcp.h"
// #include "SessionManager.h"


// Builder_OUCH_BIST::Builder_OUCH_BIST(SoupBinTcp &sbt, SessionManager &sess_mngr) noexcept : eci_local_(initialize_exc_info(sess_mngr)), sbt_(sbt) 
// {}

// Buffer_OBT* Builder_OUCH_BIST::build(const Order &order) noexcept
// {
//     constexpr std::array<size_t, 4> msg_sizes = {117, 125, 18, 17};
//     Buffer_OBT* buffer = &messages[next++ & (DAILY - 1)];
//     buffer->len = msg_sizes[static_cast<size_t>(order.message_type)];
//     char *buf = buffer->msg;
//     buf = sbt_.WriteSBTHeaderForDataPacket(buf, buffer->len);


//     (this->*OuchMessageBuilders[order.message_type])(order, buf);

//     return buffer;
// }

// std::array<ExchangeClientInfo_local, ACC_COUNT> Builder_OUCH_BIST::initialize_exc_info(SessionManager &sess_mngr) noexcept 
// {
//     std::array<ExchangeClientInfo_local, ACC_COUNT> eci_local;

//     for (size_t acc_index = 0; acc_index < ACC_COUNT; ++acc_index)
//     {
//         const auto sess_index = sess_mngr.getSessionIndex(Venue::BIST, Protocol::OUCH, acc_index);

//         const auto* sess_context = sess_mngr.getSessionContext(sess_index);
//         const size_t auth_index = sess_context->tcp_index;
//         const auto &eci = sess_mngr.getSessionAuth(auth_index)->sbt.eci;
//         eci_local[acc_index] = ExchangeClientInfo_local{std::string_view{eci.client_account.data(), eci.client_account.size()},
//                                                         std::string_view{eci.exchange_info.data(), eci.exchange_info.size()}};
//     }

//     return eci_local;
// }



//SWITCH-CASE PRE-FILL

// #include "Builder_OUCH_BIST.h"
// #include "SoupBinTcp.h"
// #include "SessionManager.h"


// Builder_OUCH_BIST::Builder_OUCH_BIST(SoupBinTcp &sbt, SessionManager &sess_mngr) noexcept : eci_local_(initialize_exc_info(sess_mngr)), sbt_(sbt) 
// {
//     // EnterOrder prefill
//     for (size_t i = 0; i < DAILY_ENTER; ++i)
//     {
//         char* buf = enter_messages[i].msg + 3; 

//         buf[0] = 'O';
//         buf[33] = 0;
//         // std::memset(buf + 50, ' ', 15);
//         Endian::write_u64_be(buf + 97, 0);
//         buf[105] = 1;
//         buf[106] = buf[107] = buf[108] = 0;
//         // std::memset(buf + 109, ' ', 3);
//         std::memset(buf + 112, 0, 2);

//     }

//     // ReplaceOrder prefill
//     for (size_t i = 0; i < DAILY_REPLACE; ++i)
//     {
//         char* buf = replace_messages[i].msg + 3;

//         buf[0] = 'U';
//         buf[41] = 0;
//         // std::memset(buf + 58, ' ', 15); 
//         Endian::write_u64_be(buf + 105, 0);
//         buf[113] = 1;
//         std::memset(buf + 114, 0, 8);

//     }

//     // Cancel prefill
//     for (size_t i = 0; i < DAILY_CANCEL; ++i)
//     {
//         cancel_messages[i].msg[3] = 'X';
//     }

//     // CancelID prefill
//     for (size_t i = 0; i < DAILY_CANCELID; ++i)
//     {
//         cancelid_messages[i].msg[3] = 'Y';
//     }
// }

// Buffer_OBT_Enter* Builder_OUCH_BIST::build_enter(const Order& order) noexcept
// {
//     auto* buffer = &enter_messages[next_enter++ & (DAILY_ENTER - 1)];
//     char* buf = buffer->msg;
//     sbt_.WriteSBTHeaderForDataPacket(buf, buffer->len);
//     buildEnterOrder(order, buf + 3);
//     return buffer;
// }

// Buffer_OBT_Replace* Builder_OUCH_BIST::build_replace(const Order& order) noexcept
// {
//     auto* buffer = &replace_messages[next_replace++ & (DAILY_REPLACE - 1)];
//     char* buf = buffer->msg;
//     sbt_.WriteSBTHeaderForDataPacket(buf, buffer->len);
//     buildReplaceOrder(order, buf + 3);
//     return buffer;
// }

// Buffer_OBT_Cancel* Builder_OUCH_BIST::build_cancel(const Order& order) noexcept
// {
//     auto* buffer = &cancel_messages[next_cancel++ & (DAILY_CANCEL - 1)];
//     char* buf = buffer->msg;
//     sbt_.WriteSBTHeaderForDataPacket(buf, buffer->len);
//     buildCancelOrder(order, buf + 3);
//     return buffer;
// }

// Buffer_OBT_CancelID* Builder_OUCH_BIST::build_cancelid(const Order& order) noexcept
// {
//     auto* buffer = &cancelid_messages[next_cancelid++ & (DAILY_CANCELID - 1)];
//     char* buf = buffer->msg;
//     sbt_.WriteSBTHeaderForDataPacket(buf, buffer->len);
//     buildCancelByOrderID(order, buf + 3);
//     return buffer;
// }

// std::array<ExchangeClientInfo_local, ACC_COUNT> Builder_OUCH_BIST::initialize_exc_info(SessionManager &sess_mngr) noexcept 
// {
//     std::array<ExchangeClientInfo_local, ACC_COUNT> eci_local;

//     for (size_t acc_index = 0; acc_index < ACC_COUNT; ++acc_index)
//     {
//         const auto sess_index = sess_mngr.getSessionIndex(Venue::BIST, Protocol::OUCH, acc_index);

//         const auto* sess_context = sess_mngr.getSessionContext(sess_index);
//         const size_t auth_index = sess_context->tcp_index;
//         const auto &eci = sess_mngr.getSessionAuth(auth_index)->sbt.eci;
//         eci_local[acc_index] = ExchangeClientInfo_local{std::string_view{eci.client_account.data(), eci.client_account.size()},
//                                                         std::string_view{eci.exchange_info.data(), eci.exchange_info.size()}};
//     }

//     return eci_local;
// }





















