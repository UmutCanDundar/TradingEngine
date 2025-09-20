#include "Limits.h"
#include "ErrorHandler.h"
#include "HashTables.h"

#include <fstream>
#include <nlohmann/json.hpp>

Limits::Limits(HashTables &hashtables) noexcept : symbol_limits_(initialize_symbollimits()), hashtables_(hashtables) {}


std::array<std::vector<SymbolLimit>, VENUE_COUNT> Limits::initialize_symbollimits() noexcept
{
    std::array<std::vector<SymbolLimit>, VENUE_COUNT> SymbolLimits{};

    std::ifstream file("../config/Limits.json");
    if (!file.is_open())
        ErrorHandler::handleError(ErrorName::CouldNotOpenFile);

    nlohmann::json j;
    file >> j;

    const auto &symbol_limits = j["symbol_limits"];
    auto venue = 0;

    for (const auto &it : symbol_limits.items())
    {
        const std::string &venue_name = it.key();
        const auto &symbols = it.value();

        auto &vec = SymbolLimits[venue];
        vec.resize(symbol_limits[venue_name].size());

        for (const auto &sym_it : symbols.items())
        {
            const std::string &symbol_string = sym_it.key();
            const auto &limits_array = sym_it.value();

            SymbolLimit sl{};
            sl.max_position_scaled = limits_array[0].get<int64_t>();
            sl.tick_size_scaled = limits_array[1].get<int64_t>();
            sl.max_notional_scaled = limits_array[2].get<int64_t>();
            sl.min_qty = limits_array[3].get<uint32_t>();
            sl.max_order_qty = limits_array[4].get<uint32_t>();
            sl.price_scale_factor = limits_array[5].get<uint32_t>();
            sl.qty_scale_factor = limits_array[6].get<uint32_t>();

            std::array<char, 8> symbol{};
            std::memcpy(symbol.data(), symbol_string.data(), symbol_string.size());
            size_t index = hashtables_.getIndex(venue, symbol);

            vec[index] = sl;
        }
        venue++;
    }

    return SymbolLimits;
}

std::array<AccountLimit, VENUE_COUNT> Limits::initialize_accountlimits() noexcept
{
    std::array<AccountLimit, VENUE_COUNT> AccountLimits{};

    std::ifstream file("../config/Limits.json");
    if (!file.is_open())
        ErrorHandler::handleError(ErrorName::CouldNotOpenFile);

    nlohmann::json j;
    file >> j;

    const auto &account_limits = j["account_limits"];
    auto venue = 0;

    for (const auto &it : account_limits.items())
    {
        const auto &account = it.value();

        AccountLimit al{};
        al.max_notional = account["max_notional"].get<int64_t>();
        al.max_position = account["max_position"].get<int64_t>();
        al.max_daily_loss = account["max_daily_loss"].get<int64_t>();
        al.max_unrealized_loss = account["max_unrealized_loss"].get<int64_t>();
        al.max_leverage = account["max_leverage"].get<double>();
        al.max_open_orders = account["max_open_orders"].get<uint32_t>();       

        AccountLimits[venue] = al;
        venue++;
    }
    
    return AccountLimits;
}