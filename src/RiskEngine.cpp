#include "RiskEngine.h"
#include "ErrorHandler.h"

#include <fstream>
#include <nlohmann/json.hpp>

RiskEngine::RiskEngine(spscOrderQueue_t &store_to_risk, spscOrderQueue_t &strategy_to_risk, spscRejectOrderQueue_t &risk_to_strategy, spscOrderQueue_t &risk_to_builder, HashTables &hashtables, MarketBook &marketbook) noexcept
    : store_to_risk_(store_to_risk), strategy_to_risk_(strategy_to_risk), risk_to_strategy_(risk_to_strategy), risk_to_builder_(risk_to_builder), hashtables_(hashtables), marketbook_(marketbook), ratelimits_(initialize_ratelimits()), symbolrisks_(initialize_symbolrisks())
{

    // for(auto &vec : symbolrisks_)
    //     vec.resize(SYMBOLS_COUNT);
    // orderrisks_map_.reserve(ORDERRISK_MAP_SIZE);
}

std::array<RateLimitBucket, VENUE_COUNT> RiskEngine::initialize_ratelimits() noexcept
{
    std::array<RateLimitBucket, VENUE_COUNT> ratelimits{};
    for (const auto &lim : VenueLimits)
    {
        Venue venue = lim.first;
        uint64_t token_capacity = lim.second.first;
        uint64_t interval_ns = lim.second.second;
        ratelimits[static_cast<std::underlying_type_t<Venue>>(lim.first)] = RateLimitBucket{token_capacity, 0, token_capacity, interval_ns};
    }
    return ratelimits;
}

std::array<std::vector<SymbolRisk>, VENUE_COUNT> RiskEngine::initialize_symbolrisks() noexcept
{
    std::array<std::vector<SymbolRisk>, VENUE_COUNT> symbolrisks{};

    std::ifstream file("../config/SymbolLimits.json");
    if (!file.is_open())
        ErrorHandler::handleError(ErrorName::CouldNotOpenFile);

    nlohmann::json j;
    file >> j;

    const auto &symbol_limits = j["symbol_limits"];
    auto venue = 0;

    for (auto it = symbol_limits.begin(); it != symbol_limits.end(); ++it, ++venue)
    {
        const std::string &venue_name = it.key();
        const auto &symbols = it.value();

        auto &vec = symbolrisks[venue];
        vec.resize(symbol_limits[venue_name].size());

        for (auto sym_it = symbols.begin(); sym_it != symbols.end(); ++sym_it)
        {
            const std::string &symbol_string = sym_it.key();
            const auto &limits_array = sym_it.value();

            SymbolRisk sr{};
            sr.max_daily_loss = limits_array[0].get<double>();
            sr.max_position_scaled = limits_array[1].get<int64_t>();
            sr.tick_size_scaled = limits_array[2].get<int64_t>();
            sr.max_notional_scaled = limits_array[3].get<int64_t>();
            sr.min_qty = limits_array[4].get<uint32_t>();
            sr.max_order_qty = limits_array[5].get<uint32_t>();

            std::array<char, 8> symbol{};
            std::memcpy(symbol.data(), symbol_string.data(), symbol_string.size());
            size_t index = hashtables_.getIndex(venue, static_cast<std::array<char, 8>>(symbol));

            vec[index] = sr;
        }
    }

    return symbolrisks;
}