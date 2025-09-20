#include "RiskEngine.h"
#include "ErrorHandler.h"

#include <fstream>
#include <nlohmann/json.hpp>

RiskEngine::RiskEngine(spscOrderQueue_t &store_to_risk, spscOrderQueue_t &strategy_to_risk, spscRejectOrderQueue_t &risk_to_strategy, spscOrderQueue_t &risk_to_builder, HashTables &hashtables, MarketBook &marketbook, Limits &limits) noexcept
    : store_to_risk_(store_to_risk), strategy_to_risk_(strategy_to_risk), risk_to_strategy_(risk_to_strategy), risk_to_builder_(risk_to_builder), hashtables_(hashtables), marketbook_(marketbook), accountrisks_(initialize_accountrisks()), symbolrisks_(initialize_symbolrisks()), limits_(limits)
{}

std::array<AccountRisk, VENUE_COUNT> RiskEngine::initialize_accountrisks() noexcept
{
    std::array<AccountRisk, VENUE_COUNT> AccountRisks{};

    std::ifstream file("../config/Limits.json");
    if (!file.is_open())
        ErrorHandler::handleError(ErrorName::CouldNotOpenFile);

    nlohmann::json j;
    file >> j;

    const auto &venue_limits = j["venue_limits"];
    auto venue = 0;

    for (const auto &it : venue_limits.items())
    {
        uint64_t token_capacity = it.value()["max_orders_per_interval"].get<uint64_t>();
        uint64_t interval_ns = it.value()["order_rate_interval_ns"].get<uint64_t>();
        uint32_t max_cancels = it.value()["max_cancels_per_sec"].get<uint32_t>();

        AccountRisks[venue].orderratelimit = OrderRateLimit{token_capacity, 0, token_capacity, interval_ns};
        AccountRisks[venue].cancelratelimit = CancelRateLimit{0, 0, max_cancels};
        venue++;
    }
    return AccountRisks;
}

std::array<std::vector<SymbolRisk>, VENUE_COUNT> RiskEngine::initialize_symbolrisks() noexcept
{
    std::array<std::vector<SymbolRisk>, VENUE_COUNT> SymbolRisks{};

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

        auto &vec = SymbolRisks[venue];
        vec.resize(symbol_limits[venue_name].size());

        for (const auto &sym_it : symbols.items())
        {
            const std::string &symbol_string = sym_it.key();
           
            SymbolRisk sr{};
           
            std::array<char, 8> symbol{};
            std::memcpy(symbol.data(), symbol_string.data(), symbol_string.size());
            size_t index = hashtables_.getIndex(venue, static_cast<std::array<char, 8>>(symbol));

            vec[index] = sr;
        }
        venue++;
    }

    return SymbolRisks;
}

