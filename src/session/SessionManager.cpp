#include "SessionManager.h"
#include "ErrorHandler.h"

#include <fstream>

#include <arpa/inet.h>
#include <nlohmann/json.hpp>

SessionManager::SessionManager() noexcept : session_contexts_(initialize_SessionContext()), session_auths_(initialize_SessionAuth()) 
{
    initialize_SessionState();
}

std::array<SessionContext, MAX_SESSIONS> SessionManager::initialize_SessionContext() noexcept
{
    std::array<SessionContext, MAX_SESSIONS> session_contexts{};

    std::ifstream file("config/Sessions.json");
    
    if (!file.is_open())
        ErrorHandler::handleError(ErrorName::CouldNotOpenFile);
            
    nlohmann::json j;
    file >> j;

    const auto &sessions = j["Sessions"];

    for (const auto &session_num : sessions.items())
    {
        auto& context = session_num.value()["SessionContext"];
        SessionContext cxt{};

        cxt.socket_index = context["socket_index"].get<size_t>();
        cxt.port = htons(context["port"].get<uint16_t>());
        cxt.ip = convertToNetworkbyte(context["ip"].get<std::string>());
        cxt.account_index = context["account_index"].get<size_t>();
        cxt.venue = convertToVenue(context["venue"].get<std::string>());
        cxt.protocol = convertToProtocol(context["protocol"].get<std::string>());
        cxt.session_type = convertToSessType(context["session_type"].get<std::string>());
        
        if (context["protocol"].get<std::string>() != "ITCH")
            cxt.tcp_index = context["tcp_index"].get<size_t>();
    
        session_contexts[cxt.socket_index] = std::move(cxt);
        session_lookup_[static_cast<size_t>(cxt.venue)][static_cast<size_t>(cxt.protocol)][cxt.account_index] = cxt.socket_index;
    }
    return session_contexts;
}

void SessionManager::initialize_SessionState() noexcept 
{

    std::ifstream file("config/Sessions.json");
    if (!file.is_open())
        ErrorHandler::handleError(ErrorName::CouldNotOpenFile);
            
    nlohmann::json j;
    file >> j;

    const auto &sessions = j["Sessions"];

    for (const auto &session_num : sessions.items())
    {
        auto& context = session_num.value()["SessionContext"];
        
        if(context["protocol"].get<std::string>() == "ITCH")
            continue;
        
        SessionProtocol prot = (context["protocol"].get<std::string>() == "FIX") ? SessionProtocol::FIX: SessionProtocol::SBT;
        
        auto index = context["socket_index"].get<size_t>();
        session_states_[index].init(prot);
    }
}

std::array<SessionAuth, MAX_TCP_SESSIONS> SessionManager::initialize_SessionAuth() noexcept
{
    std::array<SessionAuth, MAX_TCP_SESSIONS> session_auths{};

    std::ifstream file("config/Sessions.json");
    if (!file.is_open())
        ErrorHandler::handleError(ErrorName::CouldNotOpenFile);

    nlohmann::json j;
    file >> j;

    const auto &sessions = j["Sessions"];

    for (const auto &session_num : sessions.items())
    {
        auto& context = session_num.value()["SessionContext"];

        if (context["protocol"].get<std::string>() == "ITCH")
            continue;
        
        auto& auth_cxt = session_num.value()["SessionAuth"];
        auto sess_prot = auth_cxt["sess_prot"].get<std::string>();

        if (sess_prot == "FIX")
        {
            const auto username = auth_cxt["username"].get<std::string>();
            const auto password = auth_cxt["password"].get<std::string>();
            const auto my_id = auth_cxt["my_id"].get<std::string>();
            const auto venue_id = auth_cxt["venue_id"].get<std::string>();
            const auto account = auth_cxt["account"].get<std::string>();

            SessionAuth auth{
                VenueUserInfo_FIX{username, password, my_id, venue_id, account}};

            session_auths[context["tcp_index"].get<size_t>()] = std::move(auth);
        }
        else
        {
            const auto &username = auth_cxt["username"].get<std::string>();
            const auto &password = auth_cxt["password"].get<std::string>();
            const auto &cl_acc = auth_cxt["client_account"].get<std::string>();
            const auto &exc_info = auth_cxt["exchange_info"].get<std::string>();

            SessionAuth auth{
                UserInfo_SBT{username, password, cl_acc, exc_info}};

            session_auths[context["tcp_index"].get<size_t>()] = std::move(auth);
        }
    }
    return session_auths;
}

Protocol SessionManager::convertToProtocol(const std::string_view prot) noexcept
{
    if (prot == "FIX")
        return Protocol::FIX;
    if (prot == "OUCH")
        return Protocol::OUCH;
    if (prot == "ITCH")
        return Protocol::ITCH;

    return Protocol::Unknown;
}

SessionType SessionManager::convertToSessType(const std::string_view type) noexcept
{
    if (type == "Order")
        return SessionType::Order;

    return SessionType::None;
}

Venue SessionManager::convertToVenue(const std::string_view ven) noexcept
{
    if (ven == "BIST")
        return Venue::BIST;
    if (ven == "NASDAQ")
        return Venue::NASDAQ;

    return Venue::Unknown;
}

uint32_t SessionManager::convertToNetworkbyte(const std::string_view ip) noexcept
{
    in_addr addr{};
    ::inet_pton(AF_INET, ip.data(), &addr);
    return addr.s_addr; // network byte order
}