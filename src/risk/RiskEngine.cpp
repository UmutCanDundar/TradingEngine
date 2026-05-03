#include "RiskEngine.h"
#include "ErrorHandler.h"
#include "MarketBook.h"
#include "OrderManager.h"
#include "HashTables.h"

#include <fstream>
#include <cstring>

#include <nlohmann/json.hpp>

RiskEngine::RiskEngine(spscOrderQueue_t &store_to_risk, spscOrderQueue_t &strategy_to_risk, spscRejectOrderQueue_t &risk_to_strategy, spscOrderQueue_t &risk_to_builder, HashTables &hashtables, MarketBook &marketbook, Limits &limits, OrderManager &ord_mngr) noexcept
    : store_to_risk_(store_to_risk), strategy_to_risk_(strategy_to_risk), risk_to_strategy_(risk_to_strategy), risk_to_builder_(risk_to_builder), hashtables_(hashtables), marketbook_(marketbook), limits_(limits), ord_mngr_(ord_mngr)
{
        initialize_accountrisks();
        initialize_symbolrisks();
}

void RiskEngine::initialize_accountrisks() noexcept
{
    std::ifstream file("config/Limits.json");
    if (!file.is_open())
        ErrorHandler::handleError(ErrorName::CouldNotOpenFile);

    nlohmann::json j;
    file >> j;

    const auto &venue_limits = j["venue_limits"];
    auto venue = 0;

    for (const auto &it : venue_limits.items())
    {
        uint64_t token_capacity = it.value()["max_orders_per_interval"].get<uint64_t>();
        uint64_t last_refill_ns = static_cast<uint64_t>(__rdtsc() / 3e9);
        uint64_t interval_ns = it.value()["order_rate_interval_ns"].get<uint64_t>();
        uint32_t max_cancels = it.value()["max_cancels_per_sec"].get<uint32_t>();

        accountrisks_[venue].orderratelimit = OrderRateLimit{token_capacity, last_refill_ns, token_capacity, interval_ns};
        accountrisks_[venue].cancelratelimit = CancelRateLimit{0, 0, max_cancels}; 
        
        venue++;
    }
}

void RiskEngine::initialize_symbolrisks() noexcept
{
    std::ifstream file("config/Limits.json");
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

        auto &srarr = symbolrisks_[venue];

        for (const auto &sym : symbols.items())
        {
            const std::string &symbol_string = sym.key();
           
            std::array<char, SYMBOL_SIZE> symbol{};
            std::memcpy(symbol.data(), symbol_string.data(), symbol_string.size());
            size_t index = hashtables_.getIndex(venue, symbol);

            uint64_t token_capacity = sym.value()["max_orders_per_interval"].get<uint64_t>();
            uint64_t last_refill_ns = static_cast<uint64_t>(__rdtsc() / 3e9);
            uint64_t interval_ns = sym.value()["order_rate_interval_ns"].get<uint64_t>();
            uint32_t max_cancels = sym.value()["max_cancels_per_sec"].get<uint32_t>();

            srarr[index].orderratelimit = OrderRateLimit{token_capacity, last_refill_ns, token_capacity, interval_ns}; 
            srarr[index].cancelratelimit = CancelRateLimit{0, 0, max_cancels};  
        }
        venue++;
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

        if (order->isOurOrder)
        {
            update_order_risk(*order, venue_index);
            update_account_risk(accRisk, accLim, *order, metrics);
        }
        order->canModify.store(order->canModify | RISK_DONE, std::memory_order_release);

        return true;
    }

    return false;
}

bool RiskEngine::check_risk() noexcept
{
    Order *order;

    while (strategy_to_risk_.pop(order))
    {
        uint32_t RejectReason = 0;

        auto venue_index = static_cast<std::underlying_type_t<Venue>>(order->venue);
        auto &accRisk = accountrisks_[venue_index];
        auto &symRisk = symbolrisks_[venue_index][order->symbol_index];
        const auto &accLim = limits_.getAccountLimit(venue_index);
        const auto &symLim = limits_.getSymbolLimit(venue_index, order->symbol_index);
        const auto *symmeta = ord_mngr_.get_symbolmeta(order->venue, order->instrument_id);

        // ===== PRE-RISK CHECKS (FAST-FAIL)=====
        if (check_venue_halt_and_circuit(*order, *symmeta) ||
            check_order_rate_limit(accRisk, symRisk, *order))
            continue;
        
        if(UNLIKELY(order->status == Status::CancelRequest)) 
        {
            if(check_cancel_rate_limit(accRisk, symRisk, *order)) 
            {
                order->canModify.store(order->canModify | RISK_DONE, std::memory_order_relaxed);
                risk_to_builder_.push(order);
                return true;
            }
            else
            {
                continue;
            }
        }

        // ===== LIMIT-RISK CHECKS =====
        if (order->order_type == OrderType::Limit)
        {
            RejectReason |= check_price_tick_valid(order->price, *symmeta);
            RejectReason |= check_notional_value(order->price, order->quantity, symLim.max_notional_scaled);
            RejectReason |= check_market_data_and_deviation(symRisk.best_bid, symRisk.best_ask, order->price, symLim.max_price_deviation);            
            RejectReason |= check_fat_finger(order->quantity, symLim.fat_finger_qty_threshold, order->price, symRisk.best_bid, symRisk.best_ask, symLim.fat_finger_ratio);
            
            if (RejectReason){risk_to_strategy_.push(OrderWithRejectReason{order, RejectReason}); continue;}
        }

        // ===== MAIN RISK CHECKS =====
        RejectReason |= check_quantity_bounds(order->quantity, symLim.min_qty, symLim.max_qty, symmeta->round_lot_size);
        RejectReason |= check_max_open_orders_account(accRisk.open_orders_count.load(std::memory_order_acquire), accLim.max_open_orders);
        RejectReason |= check_max_open_orders_symbol(symRisk.open_orders_count.load(std::memory_order_acquire), symLim.max_open_orders);            
        RejectReason |= check_self_trade_order(*order, venue_index);
        RejectReason |= check_max_position_limit(symRisk.net_position.load(std::memory_order_acquire), order->quantity, order->side, symLim.max_position_scaled);
        RejectReason |= check_account_risk(*order, accRisk, accLim, symRisk, order->price, order->quantity);
        
        if (RejectReason){risk_to_strategy_.push(OrderWithRejectReason{order, RejectReason}); continue;}

        order->canModify.store(order->canModify | RISK_DONE, std::memory_order_relaxed);
        risk_to_builder_.push(order);

        return true;
    }

    return false;
}

// --------------------------------------HELPER UPDATE FUNCTIONS-----------------------------------------------
void RiskEngine::update_order_risk(const Order &order, const uint8_t venue_index) noexcept   
{
    if (UNLIKELY(order.cancelled_count > 1))
        return;

    OrderRiskKey key{order.symbol_index, order.price, static_cast<uint8_t>(order.side), venue_index};
    auto &oRmap = orderrisks_[venue_index];
    auto it = oRmap.find(key);
       
    if(it != oRmap.end()) 
    {
        OrderRisk *oR = it->second;

        oR->price.store(order.price, std::memory_order_release);
        oR->remaining_qty.store(order.remaining_quantity, std::memory_order_release);
        oR->status.store(order.status, std::memory_order_release);

        if (order.remaining_quantity == 0) {      
           oRmap.erase(it);
           oR->active.store(false, std::memory_order_release);
        }
    }
    else 
    {   
        OrderRisk *oR;
        do {
            oR = &orderrisk_pool_[orderrisk_next_slot & (ORDER_POOL_CAPACITY - 1)]; 
            orderrisk_next_slot++; 
        } while(UNLIKELY(oR->active.load(std::memory_order_relaxed)));  
            
        oR->price.store(order.price, std::memory_order_release);
        oR->remaining_qty.store(order.remaining_quantity, std::memory_order_release);
        oR->status.store(order.status, std::memory_order_release);
        oR->symbol_index = order.symbol_index;
        oR->side = order.side;
        oR->venue = order.venue;
        oR->active.store(true, std::memory_order_release);

        oRmap.emplace(key, oR);
    }
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
                    update_account_risk(accRisk, accLim, order, metrics);
                }
                order.status = order.StatusesPreNew[i & 1UL];
            }
            
            order.canModify.store(order.canModify | RISK_DONE, std::memory_order_release);
            return true;
        }
    }
    return false;
}

void RiskEngine::update_unrealized_pnl(SymbolRisk &symRisk, int64_t best_bid, int64_t best_ask) noexcept
{
    const int64_t net_pos = symRisk.net_position.load(std::memory_order_relaxed);

    if (net_pos == 0)
    {
        symRisk.unrealized_pnl.store(0, std::memory_order_release);
        return;
    }

    const int64_t avg_entry_price = symRisk.avg_entry_price.load(std::memory_order_relaxed);
    const int64_t net_pos_abs = std::abs(net_pos);

    if (net_pos > 0)
    {
        const int64_t pnl = (best_bid - avg_entry_price) * net_pos_abs;
        symRisk.unrealized_pnl.store(pnl, std::memory_order_release);
    }
    else
    {
        const int64_t pnl = (avg_entry_price - best_ask) * net_pos_abs;
        symRisk.unrealized_pnl.store(pnl, std::memory_order_release);
    }
}

void RiskEngine::update_symbol_risk(AccountRisk &accRisk, const Order &order, const OrderMetrics metrics, const uint8_t venue_index) noexcept
{
    const int8_t side_mult = metrics.side_mult;
    const int64_t nominal = metrics.nominal;
    const int64_t notional = metrics.notional;
    const int64_t replaced = metrics.replaced;
    auto &symRisk = symbolrisks_[venue_index][order.symbol_index];

    const int64_t exec_qty_int64 = static_cast<int64_t>(order.last_exec_quantity);

    if (order.isOurOrder)
    {
        switch (order.status)
        {
        case Status::New:
            symRisk.open_orders_count.fetch_add(1, std::memory_order_acq_rel);
            if (order.price > 0)
                symRisk.pending_notional_scaled.fetch_add(notional * side_mult, std::memory_order_release);
            break;

        case Status::Partial:
        {
            symRisk.net_position.fetch_add(side_mult * exec_qty_int64, std::memory_order_release);
            symRisk.pending_notional_scaled.fetch_sub(nominal * side_mult, std::memory_order_release);
            
            const auto net_pos = symRisk.net_position.load(std::memory_order_relaxed);

            if (side_mult == 1)
            {
                symRisk.cost_basis_scaled.fetch_add(nominal, std::memory_order_release);
                const auto avg_entry_price = symRisk.cost_basis_scaled.load(std::memory_order_relaxed) / net_pos;
                symRisk.avg_entry_price.store(avg_entry_price, std::memory_order_release);
            }
            else
            {
                auto avg_entry_price = symRisk.avg_entry_price.load(std::memory_order_relaxed);

                const auto realized_pnl = (order.price - avg_entry_price) * exec_qty_int64; 
                symRisk.realized_pnl.fetch_add(realized_pnl, std::memory_order_release);
                accRisk.daily_realized_pnl.fetch_add(realized_pnl, std::memory_order_release);
                
                if(net_pos == 0) 
                {
                    symRisk.cost_basis_scaled.store(0, std::memory_order_release);
                    symRisk.avg_entry_price.store(0, std::memory_order_release);
                }
                else
                {
                    symRisk.cost_basis_scaled.fetch_sub(avg_entry_price * exec_qty_int64, std::memory_order_release);
                }
            }
            break;
        }
        case Status::Filled:
        {
            symRisk.net_position.fetch_add(side_mult * exec_qty_int64, std::memory_order_release);
            symRisk.pending_notional_scaled.fetch_sub(nominal * side_mult, std::memory_order_release);
            symRisk.open_orders_count.fetch_sub(1, std::memory_order_release);
            
            const auto net_pos = symRisk.net_position.load(std::memory_order_relaxed);
            
            if (side_mult == 1) 
            {
                symRisk.cost_basis_scaled.fetch_add(nominal, std::memory_order_release);
                const auto avg_entry_price = symRisk.cost_basis_scaled.load(std::memory_order_relaxed) / net_pos;
                symRisk.avg_entry_price.store(avg_entry_price, std::memory_order_release);
            }
            else
            {
                auto avg_entry_price = symRisk.avg_entry_price.load(std::memory_order_relaxed);

                const auto realized_pnl = (order.price - avg_entry_price) * exec_qty_int64; 
                symRisk.realized_pnl.fetch_add(realized_pnl, std::memory_order_release);
                accRisk.daily_realized_pnl.fetch_add(realized_pnl, std::memory_order_release);
                
                if(net_pos == 0) 
                {
                    symRisk.cost_basis_scaled.store(0, std::memory_order_release);
                    symRisk.avg_entry_price.store(0, std::memory_order_release);
                }
                else
                {
                    symRisk.cost_basis_scaled.fetch_sub(avg_entry_price * exec_qty_int64, std::memory_order_release);
                }
            }
            break;
        }
        case Status::Cancelled:
        case Status::Deleted:
            if (UNLIKELY(order.cancelled_count > 1))
                return;
            
            if(order.remaining_quantity == 0)
                symRisk.open_orders_count.fetch_sub(1, std::memory_order_release);

            if (order.price > 0)
                symRisk.pending_notional_scaled.fetch_add(replaced, std::memory_order_release);
            break;

        case Status::Replaced:
            if (order.price > 0)
                symRisk.pending_notional_scaled.fetch_add(replaced, std::memory_order_release);

        case Status::Unknown:
        default:
            break;
        }
    }
    else
    {
        const int64_t best_bid = marketbook_.best_bid(*marketbook_.get_symBook(order));
        const int64_t best_ask = marketbook_.best_ask(*marketbook_.get_symBook(order));
        symRisk.best_bid.store(best_bid, std::memory_order_release);
        symRisk.best_ask.store(best_ask, std::memory_order_release);

        if (!(best_bid <= 0 && best_ask <= 0) && symRisk.cost_basis_scaled.load(std::memory_order_relaxed) != 0)
        {
            accRisk.total_unrealized_pnl.fetch_sub(symRisk.unrealized_pnl.load(std::memory_order_relaxed), std::memory_order_release);
            update_unrealized_pnl(symRisk, best_bid, best_ask);
            accRisk.total_unrealized_pnl.fetch_add(symRisk.unrealized_pnl.load(std::memory_order_relaxed), std::memory_order_release);
        }
    }

    if (UNLIKELY(symRisk.pending_notional_scaled.load(std::memory_order_relaxed) < 0))
        symRisk.pending_notional_scaled.store(0, std::memory_order_release);
    if (UNLIKELY(symRisk.open_orders_count.load(std::memory_order_relaxed) < 0))
        symRisk.open_orders_count.store(0, std::memory_order_release);
}

void RiskEngine::update_account_risk(AccountRisk &accRisk, const AccountLimit &accLim, const Order &order, const OrderMetrics metrics) noexcept
{
    const int8_t side_mult = metrics.side_mult;
    const int64_t nominal = metrics.nominal;
    const int64_t notional = metrics.notional;
    const int64_t replaced = metrics.replaced;
    const int64_t fee = order.order_type == OrderType::Market ? accLim.taker_fee_rate * nominal / 1'000'000 : accLim.maker_fee_rate * nominal / 1'000'000;

    switch (order.status)
    {

    case Status::New:
    {
        if (order.price > 0)
        {
            accRisk.current_exposure.fetch_add(notional * side_mult, std::memory_order_release);
            accRisk.used_margin.fetch_add(notional, std::memory_order_release);
        }
        accRisk.open_orders_count.fetch_add(1, std::memory_order_release);
        break;
    }

    case Status::Partial:
    {
        accRisk.balance.fetch_sub(nominal + fee, std::memory_order_release);
        accRisk.positional_exposure.fetch_add(nominal, std::memory_order_release);
        accRisk.used_margin.fetch_sub(nominal, std::memory_order_release);
        break;
    }

    case Status::Filled:
    {
        accRisk.balance.fetch_sub(nominal + fee, std::memory_order_release);
        accRisk.positional_exposure.fetch_add(nominal, std::memory_order_release);
        accRisk.used_margin.fetch_sub(nominal, std::memory_order_release);
        accRisk.open_orders_count.fetch_sub(1, std::memory_order_release);
        break;
    }

    case Status::Cancelled:
    case Status::Deleted:
    case Status::Expired:
    {
        if (UNLIKELY(order.cancelled_count > 1))
            return;
        if (order.price > 0)
        {
            accRisk.current_exposure.fetch_add(replaced * side_mult, std::memory_order_release);
            accRisk.used_margin.fetch_add(replaced, std::memory_order_release);
        }
        
        if(order.remaining_quantity == 0)
            accRisk.open_orders_count.fetch_sub(1, std::memory_order_release);

        break;
    }
    case Status::Replaced:
        if (order.price > 0)
        {
            accRisk.current_exposure.fetch_add(replaced * side_mult, std::memory_order_release);
            accRisk.used_margin.fetch_add(replaced, std::memory_order_release);
        }
        break;
    case Status::Unknown:
    default:
        break;
    }


    int64_t balance = accRisk.balance.load(std::memory_order_relaxed);
    if (balance != 0)
        accRisk.current_leverage.store((accRisk.current_exposure).load(std::memory_order_relaxed) * 10000 / balance, std::memory_order_release);
}

// --------------------------------------HELPER CHECK FUNCTIONS-----------------------------------------------
bool RiskEngine::check_venue_halt_and_circuit(Order &order, const SymbolMeta &symbolmeta) noexcept
{
    const auto &venueflags = ord_mngr_.get_venue_flags(order.venue);
    auto flags = venueflags.flags.load(std::memory_order_acquire);

    if (UNLIKELY(flags == venueflags.HALTED))
    {
        OrderWithRejectReason rej{&order, static_cast<uint32_t>(RiskRejectReason::VenueTradingHalted)};
        risk_to_strategy_.push(rej);
        return true;
    }

    if (UNLIKELY(flags == venueflags.C_BREAKER))
    {
        OrderWithRejectReason rej{&order, static_cast<uint32_t>(RiskRejectReason::CircuitBreakerTriggered)};
        risk_to_strategy_.push(rej);
        return true;
    }

    bool symbol_halted = symbolmeta.halted.load(std::memory_order_acquire);
    if (UNLIKELY(symbol_halted))
    {
        OrderWithRejectReason rej{&order, static_cast<uint32_t>(RiskRejectReason::SymbolTradingHalted)};
        risk_to_strategy_.push(rej);
        return true;
    }

    return false;
}

bool RiskEngine::check_order_rate_limit(AccountRisk &accRisk, SymbolRisk &symRisk, Order &order) noexcept
{
    static constexpr double ns_per_cycle = 1 / 3e9; // 1 cycle ≈ 0.333 ns
    uint64_t now_ns = static_cast<uint64_t>(__rdtsc() * ns_per_cycle);

    uint32_t RejectReason = 0;
    auto &orderratelimit = accRisk.orderratelimit;
    bool max_orders_for_account_reached = false;
    bool max_orders_for_symbol_reached = false;

    if (now_ns - orderratelimit.last_refill_ns > orderratelimit.refill_interval_ns)
    {
        orderratelimit.tokens = orderratelimit.capacity;
        orderratelimit.last_refill_ns = now_ns;
    }

    if (orderratelimit.tokens > 0)
    {
        orderratelimit.tokens--;
    }
    else
    {
        RejectReason = static_cast<uint32_t>(RiskRejectReason::MaxOrderRateLimitExceededAccount);
        max_orders_for_account_reached = true;
    }

    auto &sym_orderratelimit = symRisk.orderratelimit;

    if (now_ns - sym_orderratelimit.last_refill_ns > sym_orderratelimit.refill_interval_ns)
    {
        sym_orderratelimit.tokens = sym_orderratelimit.capacity;
        sym_orderratelimit.last_refill_ns = now_ns;
    }

    if (sym_orderratelimit.tokens > 0)
    {
        sym_orderratelimit.tokens--;
    }
    else
    {
        RejectReason |= static_cast<uint32_t>(RiskRejectReason::MaxOrderRateLimitExceededSymbol);
        max_orders_for_symbol_reached = true;
    }

    if (RejectReason != 0)
    {
        OrderWithRejectReason rej{&order, RejectReason};
        risk_to_strategy_.push(rej);
    }

    return max_orders_for_account_reached || max_orders_for_symbol_reached;
}

bool RiskEngine::check_cancel_rate_limit(AccountRisk &accRisk, SymbolRisk &symRisk, Order &order) noexcept
{
    
    static constexpr double ns_per_cycle = 1 / 3e9; // 1 cycle ≈ 0.333 ns
    uint64_t now_ns = static_cast<uint64_t>(__rdtsc() * ns_per_cycle);

    uint32_t RejectReason = 0;
    auto &cancelratelimit = accRisk.cancelratelimit;
    bool max_cancels_for_account_reached = false;
    bool max_cancels_for_symbol_reached = false;

    if (now_ns - cancelratelimit.last_cancel_timestamp_ns > 1'000'000'000ULL)
    {
        cancelratelimit.cancel_count_window = 0;
        cancelratelimit.last_cancel_timestamp_ns = now_ns;
    }

    cancelratelimit.cancel_count_window++;

    if (cancelratelimit.cancel_count_window > cancelratelimit.max_cancels_per_sec)
    {
        RejectReason = static_cast<uint32_t>(RiskRejectReason::MaxCancelRateLimitExceededAccount);
        max_cancels_for_account_reached = true;
    }

    auto &sym_cancelratelimit = symRisk.cancelratelimit;

    if (now_ns - sym_cancelratelimit.last_cancel_timestamp_ns > 1'000'000'000ULL)
    {
        sym_cancelratelimit.cancel_count_window = 0;
        sym_cancelratelimit.last_cancel_timestamp_ns = now_ns;
    }

    sym_cancelratelimit.cancel_count_window++;

    if (sym_cancelratelimit.cancel_count_window > sym_cancelratelimit.max_cancels_per_sec)
    {
        RejectReason |= static_cast<uint32_t>(RiskRejectReason::MaxCancelRateLimitExceededSymbol);
        max_cancels_for_symbol_reached = true;
    }

    if (RejectReason != 0)
    {
        OrderWithRejectReason rej{&order, RejectReason};
        risk_to_strategy_.push(rej);
    }

    return max_cancels_for_account_reached || max_cancels_for_symbol_reached;

}

uint32_t RiskEngine::check_price_tick_valid(int64_t price, const SymbolMeta &symbolmeta) noexcept
{

    for (auto &tick_size_entry : symbolmeta.tick_size_table)
    {
        TickSizeEntry *entry = tick_size_entry.load(std::memory_order_acquire);

        if (entry && (entry->price_to == 0 || (price >= (entry->price_from) && price <= (entry->price_to))))
        {
            int64_t tick_size = entry->tick_size;

            if (UNLIKELY(tick_size <= 0))
                return static_cast<uint32_t>(RiskRejectReason::RiskEngineInternalError);

            if (UNLIKELY((price % tick_size) != 0))
                return static_cast<uint32_t>(RiskRejectReason::PriceTickInvalid);
        }
        else
        {
            continue;
        }
    }

    return 0;
}