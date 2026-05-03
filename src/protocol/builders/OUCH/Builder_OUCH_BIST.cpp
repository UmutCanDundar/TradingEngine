#include "Builder_OUCH_BIST.h"
#include "SoupBinTcp.h"
#include "SessionManager.h"


Builder_OUCH_BIST::Builder_OUCH_BIST(SoupBinTcp &sbt, SessionManager &sess_mngr) noexcept : eci_local_(initialize_exc_info(sess_mngr)), sbt_(sbt) {}

Buffer_OBT *Builder_OUCH_BIST::build(const Order &order) noexcept
{
    constexpr std::array<size_t, 4> msg_sizes = {114, 122, 15, 14};
    Buffer_OBT *buffer = &messages[next_buffer_slot++ & (DAILY_OUCH_BIST_MSG_COUNT - 1)];
    char *buf = buffer->msg;
    buffer->len = msg_sizes[order.message_type];

    buf = sbt_.WriteSBTHeaderForDataPacket(buf, buffer->len);
    buffer->len += 3;

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

