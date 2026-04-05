
// ======================================================================================================
// SessionManager
//
// PURPOSE:
//  - Manage session contexts, states, and authentication for multiple venues and protocols.
//  - Abstracts FIX and SBT protocol session information while keeping HOT path access efficient.
//
// THREAD SAFETY:
//  - Atomic flags (`is_logged_in`, `is_connected`, `is_joined_multicast`, `logged_before`) ensure safe
//    concurrent access.
//  - ResetState() and setter functions use proper `std::memory_order` semantics for correctness.
//  - Atomic flags are separated from HOT union to prevent false sharing (cache line padding via alignas).
//
// PERFORMANCE & DESIGN NOTES:
//  - SessionState uses `union` + `alignas(64)` for HOT Sequence_FIX/Sequence_SBT to minimize cache
//    misses.
//  - Inline small accessors to avoid call overhead; conversion helpers are cold, kept out-of-line for
//    binary/code size reduction.
//
// DEVELOPER NOTES (PRE-PROFILING):
// - With MAX_SESSIONS = 8, total raw size ≈ 1.2 GB, which is **far too large for stack allocation**.
//   SessionAuth and SessionContext are relatively small, negligible in comparison. In addition,
//   due to the large size of Sequence_FIX compared to SBT, in future the union could be replaced
//   with separate structs per protocol to avoid unnecessary memory usage and improve HOT path efficiency.
// - Union is employed instead of std::variant to slightly reduce binary and cache footprint, and
//   because the small number of types keeps manual implementation straightforward.
//========================================================================================================

#pragma once

#include "Protocol-Venue.h"
#include "Sequence_FIX.h"
#include "Sequence_SBT.h"

#include <cstddef>
#include <cstdint>
#include <array>
#include <atomic>

enum class SessionType : uint8_t
{
    Order = 0,
    None = 1,
    // DropCopy = 2,
    // Backup = 3,
};

enum class SessionProtocol : uint8_t 
{
    FIX = 0,
    SBT = 1,
    None = 2,
}; 

struct VenueUserInfo_FIX 
{
    std::string username;
    std::string password;
    std::string my_id;
    std::string venue_id;
    std::string account;

    VenueUserInfo_FIX(std::string user,
                  std::string pass,
                  std::string myid, 
                  std::string venueid, 
                  std::string accountid) noexcept
        : username(std::move(user)),
          password(std::move(pass)),
          my_id(std::move(myid)), 
          venue_id(std::move(venueid)), 
          account(std::move(accountid)) 
    {
    }

    VenueUserInfo_FIX(const VenueUserInfo_FIX &) = default;
    VenueUserInfo_FIX &operator=(const VenueUserInfo_FIX &) = default;
    VenueUserInfo_FIX(VenueUserInfo_FIX &&) = default;
    VenueUserInfo_FIX &operator=(VenueUserInfo_FIX &&) = default;
    ~VenueUserInfo_FIX() = default;
};

struct ExchangeClientInfo // For OUCH BIST
{
    std::string client_account; // Client/Account for DM
    std::string exchange_info;  // Account number (first 16 used) for EQM

    ~ExchangeClientInfo() = default;
};

struct UserInfo_SBT 
{
    char username[6];
    char password[10];
    ExchangeClientInfo eci;

    UserInfo_SBT(const std::string& user,const std::string& pass,const std::string& cl_acc, const std::string& exc_info) noexcept
    {
        std::memset(username, ' ', 6);
        std::memcpy(username, user.data(), std::min(user.size(), size_t(6)));
        std::memset(password, ' ', 10);
        std::memcpy(password, pass.data(), std::min(pass.size(), size_t(10)));
        eci.client_account = cl_acc;
        eci.exchange_info = exc_info;
    }

    UserInfo_SBT(const UserInfo_SBT &) = default;
    UserInfo_SBT &operator=(const UserInfo_SBT &) = default;
    ~UserInfo_SBT() = default;
};

struct SessionAuth
{
    SessionProtocol sess_prot;

    union // Variant could be used here instead of union (Dev Comment)
    {
        UserInfo_SBT sbt;
        VenueUserInfo_FIX fix;
    };

    SessionAuth() noexcept : sess_prot(SessionProtocol::None)
    {  }

    SessionAuth(VenueUserInfo_FIX &&f) noexcept
    {
        sess_prot = SessionProtocol::FIX;
        new(&fix) VenueUserInfo_FIX(std::move(f));
    }    

    SessionAuth(UserInfo_SBT &&s) noexcept
    {
        sess_prot = SessionProtocol::SBT;
        new(&sbt) UserInfo_SBT(std::move(s));
    }

    SessionAuth(const SessionAuth &other)
        : sess_prot(other.sess_prot)
    {
        switch (other.sess_prot)
        {
        case SessionProtocol::FIX:
            new (&fix) VenueUserInfo_FIX(other.fix);
            break;
        case SessionProtocol::SBT:
            new (&sbt) UserInfo_SBT(other.sbt);
            break;
        default:
            break;
        }
    }

    SessionAuth &operator=(const SessionAuth &other)
    {
        if (this != &other)
        {
            this->~SessionAuth();
            sess_prot = other.sess_prot;

            switch (other.sess_prot)
            {
            case SessionProtocol::FIX:
                new (&fix) VenueUserInfo_FIX(other.fix);
                break;
            case SessionProtocol::SBT:
                new (&sbt) UserInfo_SBT(other.sbt);
                break;
            default:
                break;
            }
        }
        return *this;
    }

    SessionAuth(SessionAuth &&other) noexcept
        : sess_prot(other.sess_prot)
    {
        switch (other.sess_prot)
        {
        case SessionProtocol::FIX:
            new (&fix) VenueUserInfo_FIX(std::move(other.fix));
            break;
        case SessionProtocol::SBT:
            new (&sbt) UserInfo_SBT(std::move(other.sbt));
            break;
        default:
            break;
        }
        
        other.sess_prot = SessionProtocol::None;
    }

    SessionAuth &operator=(SessionAuth &&other) noexcept
    {
        if (this != &other)
        {
            this->~SessionAuth();
            sess_prot = other.sess_prot;

            switch (other.sess_prot)
            {
            case SessionProtocol::FIX:
                new (&fix) VenueUserInfo_FIX(std::move(other.fix));
                break;
            case SessionProtocol::SBT:
                new (&sbt) UserInfo_SBT(std::move(other.sbt));
                break;
            default:
                break;
            }

            other.sess_prot = SessionProtocol::None;
        }

        return *this;
    }

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
    alignas(64) // HOT
    std::atomic<bool> is_logged_in = false;  // All Atomic flags could be enum later (Dev Comment)
    SessionProtocol sess_prot;

    // HOT
    union alignas(64)
    {
        Sequence_FIX fix;
        Sequence_SBT sbt;
    };
    
    alignas(64) // COLD
    uint64_t last_login_attempt_ns{0};         
    uint32_t login_retry_count{0};
    std::atomic<bool> is_connected = false;         // All Atomic flags could be enum later (Dev Comment)
    std::atomic<bool> is_joined_multicast = false;  // All Atomic flags could be enum later (Dev Comment)
    std::atomic<bool> logged_before = false;        // All Atomic flags could be enum later (Dev Comment)

    void ResetState() noexcept
    {
        is_logged_in.store(false, std::memory_order_release);
        is_connected.store(false, std::memory_order_relaxed);
        is_joined_multicast.store(false, std::memory_order_relaxed);   
    }

    SessionState() noexcept : sess_prot(SessionProtocol::None) {}
    
    SessionState(const SessionState&) = delete;
    SessionState& operator=(const SessionState&) = delete;

    SessionState(SessionState&&) = delete;
    SessionState& operator=(SessionState&&) = delete;

    void init(SessionProtocol prot) noexcept 
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
    uint32_t ip; //Network Byte Order
    uint16_t port; // Network Byte Order
    uint8_t socket_index;
    uint8_t account_index;
    Venue venue;
    Protocol protocol;
    SessionType session_type = SessionType::Order;

    uint8_t tcp_index = 99; // Lookup index for TCP session auth
};

inline constexpr uint8_t MAX_SESSIONS = 8;
inline constexpr size_t MAX_TCP_SESSIONS = 4;
inline constexpr uint8_t ACC_COUNT = 1; // Only one account per session(protocol-venue) for now

class SessionManager
{
private:
    const std::array<SessionContext, MAX_SESSIONS> session_contexts_;
    std::array<SessionState, MAX_SESSIONS> session_states_;
    const std::array<SessionAuth, MAX_TCP_SESSIONS> session_auths_;

    std::array<
        std::array<
            std::array<size_t, ACC_COUNT>, // Only one sessiontype per session for now: SessionType::Order
            PROTOCOL_COUNT>,
        VENUE_COUNT>
        session_lookup_; // Maps (venue, protocol, account) to session index

public:

    SessionManager() noexcept;

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

    inline void setSessionLoggedIn(const uint8_t sess_index, bool is_logged_in) noexcept
    {
        SessionState* state = &session_states_[sess_index];
        state->is_logged_in.store(is_logged_in, std::memory_order_release);
        state->logged_before.store(true, std::memory_order_release);
    }
    inline void setSessionConnected(const uint8_t sess_index, bool is_connected) noexcept
    {
        SessionState* state = &session_states_[sess_index];
        state->is_connected.store(is_connected, std::memory_order_release);
    }
    inline void setSessionMulticast(const uint8_t sess_index, bool is_joined) noexcept
    {
        SessionState* state = &session_states_[sess_index];
        state->is_joined_multicast.store(is_joined, std::memory_order_release);
    }
    
    inline bool isSessionLoggedIn(const uint8_t sess_index) const noexcept
    {
        const SessionState* state = &session_states_[sess_index];
        return state->is_logged_in.load(std::memory_order_acquire);
    }
    inline bool isSessionLoggedInBefore(const uint8_t sess_index) const noexcept
    {
        const SessionState *state = &session_states_[sess_index];
        return state->logged_before.load(std::memory_order_acquire);
    }
    inline bool isSessionConnected(const uint8_t sess_index) const noexcept
    {
        const SessionState* state = &session_states_[sess_index];
        return state->is_connected.load(std::memory_order_acquire);
    }

    inline void ResetState(const uint8_t sess_index) noexcept
    {
        auto &state = session_states_[sess_index];
        state.ResetState();
    }

private:
    std::array<SessionContext, MAX_SESSIONS> initialize_SessionContext() noexcept;
    void initialize_SessionState() noexcept;
    std::array<SessionAuth, MAX_TCP_SESSIONS> initialize_SessionAuth() noexcept;

    static Protocol convertToProtocol(const std::string_view prot) noexcept;
    static SessionType convertToSessType(const std::string_view type) noexcept;
    static Venue convertToVenue(const std::string_view ven) noexcept;
    static uint32_t convertToNetworkbyte(const std::string_view ip) noexcept;
};

