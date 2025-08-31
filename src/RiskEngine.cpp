#include "RiskEngine.h"

RiskEngine::RiskEngine(spscOrderQueue_t &store_to_risk, spscOrderQueue_t &strategy_to_risk, spscRejectOrderQueue_t &risk_to_strategy, spscOrderQueue_t &risk_to_builder, HashTables &hashtables, MarketBook &marketbook) noexcept
    : store_to_risk_(store_to_risk), strategy_to_risk_(strategy_to_risk), risk_to_strategy_(risk_to_strategy), risk_to_builder_(risk_to_builder), hashtables_(hashtables), marketbook_(marketbook), ratelimits_(initialize_ratelimits())
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