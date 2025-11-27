#include "SoupBinTcp.h"

SoupBinTcp::SoupBinTcp(SessionManager &sess_mngr) noexcept : sess_mngr_(sess_mngr) {}

const std::array<SoupBinTcp::OutPacketHandlerFunc, SoupBinTcp::MAX_OUT_PACKETS> SoupBinTcp::makeOutPacketHandlersLookup() noexcept
{
    alignas(64) static std::array<OutPacketHandlerFunc, MAX_OUT_PACKETS> handlers{};

    // P — Debug
    handlers[0] = [](SessionManager&, const char*, size_t) noexcept -> std::pair<SBTAction, const char *>
    {
        return {SBTAction::Ignore, nullptr};
    };

    // A — Login Accepted
    handlers[1] = [](SessionManager  &sess_mngr_, const char *LoginAcceptedPacket, size_t sess_index) noexcept -> std::pair<SBTAction, const char *>
    {
        auto& seq_sbt = sess_mngr_.getSessionState(sess_index)->sbt;
        seq_sbt.set_sess_id(LoginAcceptedPacket + 3);
        seq_sbt.set_seq(LoginAcceptedPacket + 13);
        return {SBTAction::NotToParser, nullptr};
    };

    // J — Login Rejected
    handlers[2] = [](SessionManager&, const char *LoginRejectedPacket, size_t) noexcept -> std::pair<SBTAction, const char *>
    {
        if(LoginRejectedPacket[3] == 'S') // Session not available
            return {SBTAction::Reconnect, nullptr};
        else                              // Not Authorized
            return {SBTAction::Disconnect, nullptr};
    };

    // S —  Sequenced Data
    handlers[3] = [](SessionManager &sess_mngr_, const char *SequencedDataPacket, size_t sess_index) noexcept -> std::pair<SBTAction, const char *>
    {
        auto &seq_sbt = sess_mngr_.getSessionState(sess_index)->sbt;
        seq_sbt.increase_seq();
        return {SBTAction::ToParser, SequencedDataPacket+3};
    };

    // U — UnSequenced Data
    handlers[4] = [](SessionManager&, const char *, size_t) noexcept -> std::pair<SBTAction, const char *>
    {
        return {SBTAction::NotToParser, nullptr};
    };

    // H — HeartBeat
    handlers[5] = [](SessionManager&, const char *, size_t) noexcept -> std::pair<SBTAction, const char *>
    {
        return {SBTAction::Ignore, nullptr};
    };

    // Z — End Of Session
    handlers[6] = [](SessionManager&, const char *, size_t) noexcept -> std::pair<SBTAction, const char *>
    {
        return {SBTAction::Reconnect, nullptr};
    };

    return handlers;
}

const std::array<SoupBinTcp::OutPacketHandlerFunc, SoupBinTcp::MAX_OUT_PACKETS> SoupBinTcp::OutPacketHandlers = makeOutPacketHandlersLookup();
