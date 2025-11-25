#pragma once

#include <atomic>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstddef>

struct alignas(64) Buffer
{
    static constexpr size_t MAX_MSG_SIZE = 512;

    char data[MAX_MSG_SIZE];
    size_t len;
    char* hotdata_start;  // after header - before trailer (52 > body < 10)
    size_t hotdata_len; 
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

struct VenueUserInfo  // This information will take from config file 
{
    const char *username;
    const size_t username_len;

    const char *password;
    const size_t password_len;

    const char *my_id;
    const size_t my_id_len;

    const char *venue_id;
    const size_t venue_id_len;

    const char *account;
    const size_t acc_len;

    VenueUserInfo(const char *user, const size_t user_len,
                  const char *pass, const size_t pass_len,
                  const char *myid, const size_t myid_len,
                  const char *venueid, const size_t venueid_len,
                  const char *accountid, const size_t accountid_len) noexcept
        : username(user), username_len(user_len),
          password(pass), password_len(pass_len),
          my_id(myid), my_id_len(my_id_len),
          venue_id(venueid), venue_id_len(venue_id_len),
          account(accountid), acc_len(accountid_len)
    {
    }
};

class Session_FIX
{
private:
    const char* heartbeat_interval = "60";
    const size_t heartbeat_interval_len = 2;
    const VenueUserInfo vui_{"BIST USERNAME", 0, "BIST PASSWORD", 0, "BIST MY ID", 0, "BIST ID", 0, "ACC123", 6}; // These lines will be removed after config file mechanism is implemented
    
    static constexpr size_t DAILY_FIX_MSG_COUNT = 300'000;
    std::array<Buffer, DAILY_FIX_MSG_COUNT> messages;

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
    inline const VenueUserInfo& get_vui() const noexcept { return vui_; }
    inline Buffer* get_buffer(const size_t seqnum) noexcept { return &messages[seqnum];  }
    inline const std::array<Buffer, DAILY_FIX_MSG_COUNT>& get_history() const noexcept { return messages; }
    inline const auto get_last_resend_ts() noexcept { return sequences_.last_resend_ts; }  
};

