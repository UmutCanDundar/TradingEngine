#include "SoupBinTcp.h"

#include <cstring>

SoupBinTcp::SoupBinTcp(SessionManager &sess_mngr) noexcept : sess_mngr_(sess_mngr) {}

SBTAction SoupBinTcp::pkt_handler(const char *pkt, size_t sess_index) noexcept
{
    auto type = *(pkt + 2);

    switch (type)
    {
    case 'S':
        return handleSequencedData(sess_index);
    case 'H':
        return handleHeartbeat();
    case 'Z':
        return handleEndOfSession();
    case 'A':
        return handleLoginAccepted(pkt, sess_index);
    case 'J':
        return handleLoginRejected(pkt);
    case 'U':
        return handleUnSequencedData();
    case '+':
        return handleDebug();
    default:
        return SBTAction::Ignore;
    }
}

char* SoupBinTcp::buildFirstLoginRequest(char *buf, uint8_t sess_index) noexcept
{
    auto &sess_auth = sess_mngr_.getSessionAuth(sess_index)->sbt;

    std::memset(buf, ' ', 47);
    Endian::write_u16_be(buf, 47);
    *(buf + 2) = 'L';
    std::memcpy(buf + 3, sess_auth.username, 6);
    std::memcpy(buf + 9, sess_auth.password, 10);

    return buf;
}

char* SoupBinTcp::buildReLoginRequest(char *buf, uint8_t sess_index) noexcept
{
    auto &sess_auth = sess_mngr_.getSessionAuth(sess_index)->sbt;
    auto &seq_sbt = sess_mngr_.getSessionState(sess_index)->sbt;

    std::memset(buf, ' ', 47);
    Endian::write_u16_be(buf, 47);
    *(buf + 2) = 'L';
    std::memcpy(buf + 3, sess_auth.username, 6);
    std::memcpy(buf + 9, sess_auth.password, 10);

    size_t sess_len = std::strlen(seq_sbt.get_sess_id());
    std::memcpy(buf + 19 + (10 - sess_len), seq_sbt.get_sess_id(), sess_len);

    size_t seq_len = std::strlen(seq_sbt.get_seq());
    std::memcpy(buf + 29 + (20 - seq_len), seq_sbt.get_seq(), seq_len);

    return buf;
}

char* SoupBinTcp::buildLogoutRequest(char *buf) noexcept
{
    Endian::write_u16_be(buf, 1);
    *(buf + 2) = 'O';

    return buf;
}