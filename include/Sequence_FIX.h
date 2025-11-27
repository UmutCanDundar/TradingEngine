#pragma once

#include <atomic>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstddef>

struct alignas(64) Buffer_FIX
{
    static constexpr size_t MAX_MSG_SIZE = 512;

    char data[MAX_MSG_SIZE];
    uint16_t len;
    char* hotdata_start;  // after header - before trailer (52 > body < 10)
    uint16_t hotdata_len; 
};

struct FIXSequence {

    alignas(64) // builder thread
    std::atomic<uint32_t> my_next_seqnum{1};

    alignas(64) // parser thread
    std::chrono::steady_clock::time_point last_resend_ts = std::chrono::steady_clock::now();
    uint32_t expected_seqnum{1};
    uint8_t resend_counter = 0;


    static constexpr std::chrono::seconds RESEND_INTERVAL{1};
    static constexpr uint8_t MAX_RESEND_ATTEMPT = 3;
};

class Sequence_FIX
{
private:
    const char *heartbeat_interval = "60";
    const size_t heartbeat_interval_len = 2;

    static constexpr size_t DAILY_FIX_MSG_COUNT = 300'000;
    std::array<Buffer_FIX, DAILY_FIX_MSG_COUNT> messages;

    FIXSequence sequences_;

public:
    inline void set_next_seq(uint32_t num) noexcept { sequences_.my_next_seqnum.store(num, std::memory_order_release); }
    inline void set_expected_seq(uint32_t num) noexcept { sequences_.expected_seqnum = num; }
    inline void set_last_resend_ts(auto now_ts) noexcept { sequences_.last_resend_ts = now_ts; }
    inline void reset_resend_counter() noexcept { sequences_.resend_counter = 0; }

    inline void increase_next_seq() noexcept { sequences_.my_next_seqnum.fetch_add(1, std::memory_order_release); }
    inline void increase_expected_seq() noexcept { sequences_.expected_seqnum++; }
    inline void increase_resend_counter() noexcept { sequences_.resend_counter++; }

    inline uint32_t get_next_seq() noexcept { return sequences_.my_next_seqnum.load(std::memory_order_acquire); }
    inline uint32_t get_expected() noexcept { return sequences_.expected_seqnum; }
    inline uint8_t get_resend_counter() noexcept { return sequences_.resend_counter; }
    inline const char* get_interval() const noexcept { return heartbeat_interval; }
    inline const size_t get_interval_len() const noexcept { return heartbeat_interval_len; }
    inline Buffer_FIX* get_buffer(const size_t seqnum) noexcept { return &messages[seqnum];  }
    inline const std::array<Buffer_FIX, DAILY_FIX_MSG_COUNT>& get_history() const noexcept { return messages; }
    inline const auto get_last_resend_ts() noexcept { return sequences_.last_resend_ts; }  
};

