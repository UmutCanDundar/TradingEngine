#pragma once

#include "Protocol-Venue.h"
#include "common.h"
#include <cstddef>
#include <cstdint>
#include <array>
#include <atomic>
#include "Sequence_FIX.h"
#include "Sequence_SBT.h"

enum class SessionType : uint8_t
{
    Order = 0,
    // DropCopy = 1,
    // MarketData = 2,
    // Backup = 3,
};

enum class SessionProtocol : uint8_t 
{
    FIX = 0,
    SBT = 1,
    None = 2,
}; 

struct VenueUserInfo_FIX // This information will take from config file
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

    VenueUserInfo_FIX(const char *user, const size_t user_len,
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

struct UserInfo_SBT // This information will take from config file
{
    char username[6];
    char password[10];

    UserInfo_SBT(const char *user, const char *pass) noexcept
    {
        std::memset(username, ' ', 6);
        std::memcpy(username, user, std::min(strlen(user), size_t(6)));
        std::memset(password, ' ', 10);
        std::memcpy(password, pass, std::min(strlen(pass), size_t(10)));
    }
};

struct SessionAuth
{
    SessionProtocol sess_prot;

    union
    {
        UserInfo_SBT sbt;
        VenueUserInfo_FIX fix;
    };

    SessionAuth(SessionProtocol sess_prot, VenueUserInfo_FIX &fix) : sess_prot(SessionProtocol::FIX), fix(fix) {}
    SessionAuth(SessionProtocol sess_prot, UserInfo_SBT &sbt) : sess_prot(SessionProtocol::SBT), sbt(sbt) {}
    
    ~SessionAuth() 
    {
        switch(sess_prot) 
        {
            case SessionProtocol::FIX:
                fix.~VenueUserInfo_FIX();
                break;
            case SessionProtocol::SBT:
                sbt.~UserInfo_SBT();
                break;
            default:
                break;
        }
    }
};

struct SessionState
{
    std::atomic<bool> is_logged_in = false;    // These could be single enum var
    std::atomic<bool> is_connected = false;
    std::atomic<bool> is_joined_multicast = false;

    std::atomic<bool> logged_before = false;
    SessionProtocol sess_prot;

     union 
    {
        Sequence_FIX fix;
        Sequence_SBT sbt;
    };

    void ResetState() noexcept
    {
        is_logged_in.store(false, std::memory_order_release);
        is_connected.store(false, std::memory_order_release);
        is_joined_multicast.store(false, std::memory_order_release);   
    }

    SessionState(SessionProtocol prot) : sess_prot(prot)
    {
        switch (prot)
        {
        case SessionProtocol::FIX:
            new (&fix) Sequence_FIX();
            break;
        case SessionProtocol::SBT:
            new (&sbt) Sequence_SBT();
            break;
        default:
            break;       
        }
    }

    ~SessionState()
    {
        switch (sess_prot)
        {
        case SessionProtocol::FIX:
            fix.~Sequence_FIX();
            break;
        case SessionProtocol::SBT:
            sbt.~Sequence_SBT();
            break;
        default:
            break;
        }
    }
};

struct SessionContext 
{
    uint8_t socket_index;
    uint8_t port;
    uint8_t ip;
    uint8_t account_index;
    Venue venue;
    Protocol protocol;
    SessionType session_type = SessionType::Order;

    uint8_t tcp_index = 99; // Lookup index for TCP session auth
};

inline constexpr uint8_t MAX_SESSIONS = 8;

class SessionManager
{
private:
    static constexpr uint8_t ACC_COUNT = 1; // Only one account per session for now
    static constexpr size_t MAX_TCP_SESSIONS = 4;

    std::array<const SessionContext, MAX_SESSIONS> session_contexts_;
    std::array<SessionState, MAX_SESSIONS> session_states_;
    std::array<const SessionAuth, MAX_TCP_SESSIONS> session_auths_;

    std::array<
        std::array<
            std::array<size_t, ACC_COUNT>, // Only one session per Account for now: SessionType::Order
        PROTOCOL_COUNT>,
    VENUE_COUNT> session_lookup_; // Maps (venue, protocol, account) to session index


public:
    inline size_t getSessionIndex(Venue venue, Protocol protocol, uint8_t account_index = 0) const noexcept
    { 
        return session_lookup_[static_cast<size_t>(venue)][static_cast<size_t>(protocol)][account_index];
    }

    inline const SessionContext* getSessionContext(uint8_t sess_index) const noexcept
    {
        return &session_contexts_[sess_index];
    }

    inline SessionState* getSessionState(uint8_t sess_index) noexcept
    {
        return &session_states_[sess_index];
    }

    inline const SessionAuth* getSessionAuth(uint8_t auth_index) const noexcept
    {
        return &session_auths_[auth_index];
    }

    inline void setSessionLoggedIn(uint8_t sess_index, bool is_logged_in) noexcept
    {
        SessionState* state = &session_states_[sess_index];
        state->is_logged_in.store(is_logged_in, std::memory_order_release);
        state->logged_before.store(true, std::memory_order_release);
    }

    inline void setSessionConnected(uint8_t sess_index, bool is_connected) noexcept
    {
        SessionState* state = &session_states_[sess_index];
        state->is_connected.store(is_connected, std::memory_order_release);
    }

    inline void setSessionMulticast(uint8_t sess_index, bool is_joined) noexcept
    {
        SessionState* state = &session_states_[sess_index];
        state->is_joined_multicast.store(is_joined, std::memory_order_release);
    }

    inline bool isSessionLoggedIn(uint8_t sess_index) const noexcept
    {
        const SessionState* state = &session_states_[sess_index];
        return state->is_logged_in.load(std::memory_order_acquire);
    }

    inline bool isSessionLoggedInBefore(uint8_t sess_index) const noexcept
    {
        const SessionState *state = &session_states_[sess_index];
        return state->logged_before.load(std::memory_order_acquire);
    }

    inline bool isSessionConnected(uint8_t sess_index) const noexcept
    {
        const SessionState* state = &session_states_[sess_index];
        return state->is_connected.load(std::memory_order_acquire);
    }

    inline void ResetState(size_t sess_index) noexcept
    {
        auto &state = session_states_[sess_index];
        state.ResetState();
    }
};

