#pragma once

#include "GeneratedSBTPackets.h"
#include "MessagePool.h"
#include "Endian.h"
#include "SessionManager.h"

#include <array>
#include <cstdint>
#include <cstddef>
#include <variant>
#include <cstring>
#include <utility>

enum class SBTAction : uint8_t {
    NotToParser = 0,
    ToParser = 1,
    Disconnect = 2,
    Reconnect = 3,
    Ignore = 4,
};

struct Buffer_SBT {
    static constexpr size_t MAX_PKT_SIZE = 64;
    
    char pkt[MAX_PKT_SIZE];
    uint16_t len;
};

class SoupBinTcp
{
private:
    static constexpr size_t DAILY_SBT_SESSION_COUNT = 64;
    std::array<Buffer_SBT, DAILY_SBT_SESSION_COUNT> sess_bufs;
    size_t next_buffer_slot = 0;

    static constexpr size_t MAX_OUT_PACKETS = 7;
    using OutPacketHandlerFunc = std::pair<SBTAction, const char*> (*)(SessionManager&, const char*, size_t) noexcept;
    static const std::array<OutPacketHandlerFunc, MAX_OUT_PACKETS> makeOutPacketHandlersLookup() noexcept;
    static const std::array<OutPacketHandlerFunc, MAX_OUT_PACKETS> OutPacketHandlers;
    
    SessionManager &sess_mngr_;
    
public:
    SoupBinTcp(SessionManager &sess_mngr) noexcept;

    inline std::pair<SBTAction, const char*> pkt_handler(const char* pkt, size_t sess_index) noexcept
    {
        size_t index = static_cast<size_t>(PacketIndex(*(pkt+2)));
        if (index != 99)
            return OutPacketHandlers[index](sess_mngr_, pkt, sess_index);

        return {SBTAction::Ignore, nullptr};
    }

    inline char* WriteSBTHeaderForDataPacket(char *buf, uint16_t OUCHmsg_len) noexcept
    {
        Endian::write_u16_be(buf, OUCHmsg_len + 1);
        *(buf + 2) = 'U';

        return buf + 3;
    }

    inline char *buildFirstLoginRequest(char *buf, uint8_t sess_index) noexcept
    {
        auto &sess_auth = sess_mngr_.getSessionAuth(sess_index)->sbt;

        std::memset(buf, ' ', 47);
        Endian::write_u16_be(buf, 47);
        *(buf + 2) = 'L';
        std::memcpy(buf + 3, sess_auth.username, 6);
        std::memcpy(buf + 9, sess_auth.password, 10);

        return buf;
    }

    inline char *buildReLoginRequest(char *buf, uint8_t sess_index) noexcept
    {
        auto& sess_auth = sess_mngr_.getSessionAuth(sess_index)->sbt;
        auto& seq_sbt = sess_mngr_.getSessionState(sess_index)->sbt;

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

    inline char *buildHeartbeat(char *buf) noexcept
    {
        Endian::write_u16_be(buf, 1);
        *(buf + 2) = 'R';

        return buf;
    }

    inline char *buildLogoutRequest(char *buf) noexcept
    {
        Endian::write_u16_be(buf, 1);
        *(buf + 2) = 'O';

        return buf;
    }
};

