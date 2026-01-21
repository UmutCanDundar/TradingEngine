#include "RiskEngine.h"
#include "ErrorHandler.h"

#include <fstream>
#include <nlohmann/json.hpp>

RiskEngine::RiskEngine(spscOrderQueue_t &store_to_risk, spscOrderQueue_t &strategy_to_risk, spscRejectOrderQueue_t &risk_to_strategy, spscOrderQueue_t &risk_to_builder, HashTables &hashtables, MarketBook &marketbook, Limits &limits, Store_RAM &store_ram) noexcept
    : store_to_risk_(store_to_risk), strategy_to_risk_(strategy_to_risk), risk_to_strategy_(risk_to_strategy), risk_to_builder_(risk_to_builder), hashtables_(hashtables), marketbook_(marketbook), limits_(limits), store_ram_(store_ram)
{}
void RiskEngine::initialize_accountrisks() noexcept
{
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

        accountrisks_[venue].orderratelimit = OrderRateLimit{token_capacity, 0, token_capacity, interval_ns};
        accountrisks_[venue].cancelratelimit = CancelRateLimit{0, 0, max_cancels}; 
        
        venue++;
    }
}

void RiskEngine::initialize_symbolrisks() noexcept
{
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

        auto &srvec = symbolrisks_[venue];
        srvec.resize(symbol_limits[venue_name].size());

        for (const auto &sym_it : symbols.items())
        {
            const std::string &symbol_string = sym_it.key();
           
            std::array<char, SYMBOL_SIZE> symbol{};
            std::memcpy(symbol.data(), symbol_string.data(), symbol_string.size());
            size_t index = hashtables_.getIndex(venue, symbol);

            uint64_t token_capacity = it.value()["max_orders_per_interval"].get<uint64_t>();
            uint64_t interval_ns = it.value()["order_rate_interval_ns"].get<uint64_t>();
            uint32_t max_cancels = it.value()["max_cancels_per_sec"].get<uint32_t>();

            srvec[index].orderratelimit = OrderRateLimit{token_capacity, 0, token_capacity, interval_ns}; 
            srvec[index].cancelratelimit = CancelRateLimit{0, 0, max_cancels};  
        }
        venue++;
    }
}

inline void RiskEngine::update_order_risk(const Order &order, const uint8_t venue_index) noexcept   
{
    if (UNLIKELY(order.cancelled_count > 0))
        return;

    OrderRiskKey key{order.symbol_index, order.price, static_cast<uint8_t>(order.side)};
    auto &oRmap = orderrisks_[venue_index];
    auto it = oRmap.find(key);
       
    if(it != oRmap.end()) 
    {
        OrderRisk *oR = it->second;

        oR->price_scaled.store(order.price, std::memory_order_release);
        oR->remaining_qty.store(std::max<uint32_t>(order.quantity - order.filled_quantity, 0), std::memory_order_release);
        oR->status.store(order.status, std::memory_order_release);

        constexpr uint64_t terminal_mask =
        (1ULL << static_cast<uint64_t>(Status::Cancelled))
        | (1ULL << static_cast<uint64_t>(Status::Expired));

        if (oR->remaining_qty.load(std::memory_order_relaxed) == 0 || (terminal_mask & (1ULL << static_cast<uint64_t>(order.status)))) {      
           oRmap.erase(it);
           oR->active.store(false, std::memory_order_release);
        }
    
    }
    else // insert new entry
    {   
        OrderRisk *oR;
        do {

            oR = &orderrisk_pool_[orderrisk_next_slot & (ORDER_POOL_CAPACITY - 1)]; 
            orderrisk_next_slot++; 

        } while(UNLIKELY(oR->active.load(std::memory_order_relaxed)));  
            
       
        oR->price_scaled.store(order.price, std::memory_order_release);
        oR->remaining_qty.store(std::max<uint32_t>(order.quantity - order.filled_quantity, 0), std::memory_order_release);
        oR->status.store(order.status, std::memory_order_release);
        oR->symbol_index = order.symbol_index;
        oR->side = order.side;
        oR->active.store(true, std::memory_order_release);

        oRmap.emplace(key, oR);
    }
}

bool RiskEngine::update_risk() noexcept
{
    Order *order;
    while (store_to_risk_.pop(order))
    {
        const uint8_t venue_index = static_cast<uint8_t>(order->venue);
        auto &accRisk = accountrisks_[venue_index];
        const auto &accLim = limits_.getAccountLimit(venue_index);
        const OrderMetrics metrics(*order);

        if (update_risk_for_protocol_fix(accRisk, accLim, *order, metrics))
            continue;

        update_symbol_risk(accRisk, *order, metrics, venue_index);

        if (order->isOurOrder) {
            update_order_risk(*order, venue_index);
            update_account_risk(accRisk, accLim, *order, metrics, venue_index);
        }
        order->canModify.store(order->canModify | RISK_DONE, std::memory_order_release);

        return true;
    }    

    return false;
}

 bool RiskEngine::update_risk_for_protocol_fix(AccountRisk &accRisk, const AccountLimit &accLim, Order &order, const OrderMetrics metrics) noexcept
    {
        if (order.protocol == Protocol::FIX) {
            if(UNLIKELY(order.syncState == SyncState::WaitingNew))
            {
                return true;
            }
            else 
            {
                const uint8_t venue_index = static_cast<uint8_t>(order.venue);
                for(size_t i = 0; i <= order.StatusesPreNew.size(); i++)
                {
                    if(order.status != Status::Unknown){
                        update_symbol_risk(accRisk, order, metrics, venue_index);
                        update_order_risk(order, venue_index);
                        update_account_risk(accRisk, accLim, order, metrics, venue_index);
                    }
                    order.status = order.StatusesPreNew[i & 1UL];
                }
                
                order.canModify.store(order.canModify | RISK_DONE, std::memory_order_release);
                return true;
            }
        }
        return false;
    }

    bool RiskEngine::check_risk() noexcept
    {
        Order *order;
        
        while (strategy_to_risk_.pop(order))
        {
            uint32_t RejectReason = 0;

            uint8_t venue_index = static_cast<std::underlying_type_t<Venue>>(order->venue);
            auto& accRisk = accountrisks_[venue_index];
            auto& symRisk = symbolrisks_[venue_index][hashtables_.getIndex(venue_index, order->symbol)];
            const auto& accLim = limits_.getAccountLimit(venue_index);
            const auto& symLim = limits_.getSymbolLimit(venue_index, hashtables_.getIndex(venue_index, order->symbol));
            const auto* symmeta = store_ram_.get_symbolmeta(order->venue, order->instrument_id);

                // ===== PRE-RISK CHECKS =====
                if (check_venue_halt_and_circuit(*order, *symmeta) ||
                    check_order_rate_limit(accRisk, symRisk, *order, venue_index) ||
                    check_cancel_rate_limit(accRisk, symRisk, *order, venue_index)) // check_duplicate_order_id(this->order_ids, *order, venue_index))
                continue;

                // ===== LIMIT-RISK CHECKS =====
                if (order->order_type == OrderType::Limit)
                { 
                    RejectReason |=
                        check_price_tick_valid(order->price, *symmeta) |
                        check_notional_value(order->price, order->quantity, symLim.max_notional_scaled) |
                        check_market_data_and_price_band_helper(symRisk.best_bid, symRisk.best_ask, order->price, symLim.max_price_deviation) |
                        check_fat_finger_price_ratio(order->price, symRisk.best_bid, symRisk.best_ask, symLim.fat_finger_ratio);
                }

                // ===== MAIN RISK CHECKS =====
                RejectReason |=
                    check_quantity_bounds(order->quantity, symLim.min_qty, symLim.max_qty, symmeta->round_lot_size) |
                    check_max_open_orders_account(accRisk.open_orders_count.load(std::memory_order_acquire), accLim.max_open_orders) |
                    check_max_open_orders_symbol(symRisk.open_orders_count.load(std::memory_order_acquire), symLim.max_open_orders) |
                    check_self_trade_order(this->orderrisks_, *order, venue_index) |
                    check_max_position_limit(symRisk.net_position_scaled.load(std::memory_order_acquire), symLim.max_position_scaled) |
                    check_account_risk(*order, accRisk, accLim, order->price, order->quantity) |
                    check_fat_finger_quantity(order->quantity, symLim.fat_finger_qty_threshold);

                if (RejectReason != 0)
                {
                    risk_to_strategy_.push(OrderWithRejectReason{*order, RejectReason});
                    continue;
                }
                

            order->canModify.store(order->canModify | RISK_DONE, std::memory_order_relaxed);   
            risk_to_builder_.push(order);
            
            return true;
        }

        return false;
    }
