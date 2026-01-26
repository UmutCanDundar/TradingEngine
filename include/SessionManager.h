#pragma once

#include "Protocol-Venue.h"
#include "common.h"
#include <cstddef>
#include <cstdint>
#include <array>
#include <atomic>
#include "Sequence_FIX.h"
#include "Sequence_SBT.h"
#include "ErrorHandler.h"
#include <arpa/inet.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <chrono>

enum class SessionType : uint8_t
{
    Order = 0,
    None = 1,
    // DropCopy = 1,
    // Backup = 2,
};

enum class SessionProtocol : uint8_t 
{
    FIX = 0,
    SBT = 1,
    None = 2,
}; 

struct VenueUserInfo_FIX // This information will be taken from config file
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
};

struct UserInfo_SBT // This information will be taken from config file
{
    char username[6];
    char password[10];

    UserInfo_SBT(std::string user, std::string pass) noexcept
    {
        std::memset(username, ' ', 6);
        std::memcpy(username, user.data(), std::min(user.size(), size_t(6)));
        std::memset(password, ' ', 10);
        std::memcpy(password, pass.data(), std::min(pass.size(), size_t(10)));
    }

    UserInfo_SBT(const UserInfo_SBT &) = default;
    UserInfo_SBT &operator=(const UserInfo_SBT &) = default;
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
    std::atomic<bool> is_logged_in = false;    // All Atomic flags could be enum later (Dev Comment)
    SessionProtocol sess_prot;

    // HOT
    union alignas(64) // Variant could be used here instead of union (Dev Comment)
    {
        Sequence_FIX fix;
        Sequence_SBT sbt;
    };
    
    alignas(64) // COLD
    uint64_t last_login_attempt_ns{0};         
    uint32_t login_retry_count{0};
    std::atomic<bool> is_connected = false;
    std::atomic<bool> is_joined_multicast = false;
    std::atomic<bool> logged_before = false;
     
   
    void ResetState() noexcept
    {
        is_logged_in.store(false, std::memory_order_release);
        is_connected.store(false, std::memory_order_relaxed);
        is_joined_multicast.store(false, std::memory_order_relaxed);   
    }

    SessionState() noexcept : sess_prot(SessionProtocol::None) {}

    SessionState(SessionProtocol prot) noexcept : sess_prot(prot)
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

class SessionManager
{
private:
    static constexpr uint8_t ACC_COUNT = 1; // Only one account per session for now
    static constexpr size_t MAX_TCP_SESSIONS = 4;

    const std::array<SessionContext, MAX_SESSIONS> session_contexts_;
    std::array<SessionState, MAX_SESSIONS> session_states_;
    const std::array<SessionAuth, MAX_TCP_SESSIONS> session_auths_;

    std::array<
        std::array<
            std::array<size_t, ACC_COUNT>, // Only one session per Account for now: SessionType::Order
        PROTOCOL_COUNT>,
    VENUE_COUNT> session_lookup_; // Maps (venue, protocol, account) to session index

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

    std::array<SessionContext, MAX_SESSIONS> initialize_SessionContext() noexcept
    {
        std::array<SessionContext, MAX_SESSIONS> session_contexts;
        
        std::ifstream file("../config/Sessions.json");
        if (!file.is_open())
            ErrorHandler::handleError(ErrorName::CouldNotOpenFile);

        nlohmann::json j;
        file >> j;

        const auto &sessions = j["Sessions"];
        
        for (const auto &session_num : sessions.items())
        {
            SessionContext cxt{};

            for (const auto &session : session_num.value())
            {
                auto& context = session["SessionContext"];

                cxt.socket_index = context["socket_index"].get<size_t>();
                cxt.port = htons(context["port"].get<uint16_t>());
                cxt.ip = convertToNetworkbyte(context["ip"].get<std::string>());
                cxt.account_index = context["account_index"].get<size_t>();
                cxt.venue = convertToVenue(context["venue"].get<std::string>());
                cxt.protocol = convertToProtocol(context["protocol"].get<std::string>());
                cxt.session_type = convertToSessType(context["session_type"].get<std::string>());
                cxt.tcp_index = context["tcp_index"].get<size_t>();
            }

            session_contexts[cxt.socket_index] = cxt;
        }

        return session_contexts;
    }

    std::array<SessionAuth, MAX_TCP_SESSIONS> initialize_SessionAuth() noexcept
    {
        std::array<SessionAuth, MAX_TCP_SESSIONS> session_auths{};

        std::ifstream file("../config/Sessions.json");
        if (!file.is_open())
            ErrorHandler::handleError(ErrorName::CouldNotOpenFile);

        nlohmann::json j;
        file >> j;

        const auto &sessions = j["Sessions"];

        for (const auto &session_num : sessions.items())
        {
            for (const auto &session : session_num.value())
            {
                auto context = session["SessionContext"];

                if(context["protocol"] == "ITCH")
                    break;
                else
                {
                    auto auth_cxt = session["SessionAuth"];
                    auto sess_prot = auth_cxt["sess_prot"].get<std::string>();

                    if (sess_prot == "FIX")
                    {
                        const auto username = auth_cxt["username"].get<std::string>();
                        const auto password = auth_cxt["password"].get<std::string>();
                        const auto my_id = auth_cxt["my_id"].get<std::string>();
                        const auto venue_id = auth_cxt["venue_id"].get<std::string>();
                        const auto account = auth_cxt["account"].get<std::string>();
                    
                        SessionAuth auth
                        {
                            VenueUserInfo_FIX{username, password, my_id, venue_id, account}
                        };

                        session_auths[context["tcp_index"].get<size_t>()] = std::move(auth);
                    }
                    else
                    {
                        const auto &username = auth_cxt["username"].get<std::string>();
                        const auto &password = auth_cxt["password"].get<std::string>();
                        
                        SessionAuth auth
                        {
                            UserInfo_SBT{username, password}
                        };

                        session_auths[context["tcp_index"].get<size_t>()] = std::move(auth);
                    }
                }
            }
        }
        return session_auths;
    }

    private:
        static Protocol convertToProtocol(const std::string_view prot) noexcept
        {
            if (prot == "FIX")
                return Protocol::FIX;
            if (prot == "OUCH")
                return Protocol::OUCH;
            if (prot == "ITCH")
                return Protocol::ITCH;

            return Protocol::Unknown;
        }

        static SessionType convertToSessType(const std::string_view type) noexcept
        {
            if (type == "Order")
                return SessionType::Order;

            return SessionType::None;
        }

        static Venue convertToVenue(const std::string_view ven) 
        {
            if(ven == "BIST")
                return Venue::BIST;
            if(ven == "NASDAQ")
                return Venue::NASDAQ;

            return Venue::Unknown;
        }

        static uint32_t convertToNetworkbyte(const std::string_view ip) noexcept
        {
            in_addr addr{};
            ::inet_pton(AF_INET, ip.data(), &addr);
            return addr.s_addr; // network byte order
        }
};

