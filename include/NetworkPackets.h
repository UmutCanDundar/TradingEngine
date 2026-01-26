#pragma once

#include <cstdint>
#include <array>
#include <cstring>
#include "Protocol-Venue.h"
#include <boost/lockfree/spsc_queue.hpp>

struct InPacket // Inbound Message Struct
{
    static constexpr size_t DATA_SIZE = 2048;

    std::array<char, DATA_SIZE> data;
    uint16_t len;
    uint16_t offset;
    uint8_t sock_index;
    bool is_login_msg = false;

    InPacket() noexcept = default;
    InPacket(char *msg, size_t len, uint8_t sock_index, bool is_login = false) noexcept : len(len), offset(0), sock_index(sock_index), is_login_msg(is_login)
    {
        std::memcpy(data.data(), msg, len);
    }

    inline void fillPacket(const char *src, uint16_t src_len, uint8_t session_index, bool login_msg = false) noexcept
    {
        std::memcpy(data.data(), src, src_len);
        len = src_len;
        offset = 0;
        sock_index = session_index;
        is_login_msg = login_msg;
    }
};

struct OutPacket // Outbound Message Struct
{
    static constexpr size_t DATA_SIZE = 2048;
    static constexpr size_t MAX_APP_MSG_IN_ONE_DATA = 32;

    std::array<char, DATA_SIZE> data;
    std::array<uint16_t, MAX_APP_MSG_IN_ONE_DATA> offsets;
    uint16_t len;
    uint16_t last_pkt_remaining_len = 0;
    uint8_t msg_count = 0;
    bool half = false;
    bool is_len_received = true;
    Venue venue;
    Protocol protocol;
};

inline constexpr size_t PACKET_QUEUE_CAPACITY = 1024;
using spscInPacketQueue_t = boost::lockfree::spsc_queue<InPacket *, boost::lockfree::capacity<PACKET_QUEUE_CAPACITY>>;
using spscPacketQueue_t = boost::lockfree::spsc_queue<OutPacket *, boost::lockfree::capacity<PACKET_QUEUE_CAPACITY>>;

inline constexpr size_t PACKET_POOL_CAPACITY = 1024;

class InPacketPoolManager
{
private:
    std::array<InPacket, PACKET_POOL_CAPACITY> pool{};
    size_t next_inpkt{0};
    
public:

    InPacket *get_inpkt() noexcept
    {
        return &pool[next_inpkt++ & (PACKET_POOL_CAPACITY - 1)];
    }

};