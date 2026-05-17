#include "OrderManager.h"
#include "MarketBook.h"
#include "HashTables.h"
#include "Order.h"
#include "dataset_risk.h"
#include "dataset_order_manager.h"
#include "RiskEngine.h"
#include "Limits.h"
#include "Logger.h"
#include "perf_utils.h"

#include <thread>
#include <atomic>
#include <memory>

inline void reset_accountrisk(AccountRisk& ar) noexcept
{
    ar.current_exposure.store(0, std::memory_order_relaxed);
    ar.balance.store(10'000'000, std::memory_order_relaxed);
    ar.positional_exposure.store(0, std::memory_order_relaxed);
    ar.used_margin.store(0, std::memory_order_relaxed);
    ar.current_leverage.store(1, std::memory_order_relaxed);
    ar.daily_realized_pnl.store(0, std::memory_order_relaxed);
    ar.total_unrealized_pnl.store(0, std::memory_order_relaxed);
    ar.open_orders_count.store(0, std::memory_order_relaxed);
}

inline void reset_symbolrisk(SymbolRisk& sr) noexcept
{
    sr.net_position.store(0, std::memory_order_relaxed);
    sr.cost_basis_scaled = 0;
    sr.unrealized_pnl = 0;
    sr.realized_pnl = 0;
    sr.pending_notional_scaled.store(0, std::memory_order_relaxed);
    sr.best_bid.store(10000, std::memory_order_relaxed);
    sr.best_ask.store(10000, std::memory_order_relaxed);
    sr.avg_entry_price.store(0, std::memory_order_relaxed);
    sr.open_orders_count.store(0, std::memory_order_relaxed);
}

int main()
{
    pin_to_cpu(2);

    std::unique_ptr<HashTables>    hashtables;
    std::unique_ptr<Limits>        limits;
    std::unique_ptr<MarketBook>    marketbook;
    std::unique_ptr<OrderManager>  order_manager;
    std::unique_ptr<RiskEngine>    risk;

    spscMessageQueue_t        parser_to_store;
    spscOrderQueue_t          strategy_to_risk;
    spscRejectOrderQueue_t    risk_to_strategy;
    spscOrderQueue_t          risk_to_builder;
    spscOrderQueue_t          store_to_strategy_free_slot;
    spscOrderQueue_t          store_to_risk;
    spscOrderQueue_t          store_to_strategy;
    spscDbQueue_t             store_to_db;

    std::atomic<bool> stop{false};
    std::atomic<bool> stop2{false};

    std::thread consumer;
    std::thread consumer2;

    hashtables    = std::make_unique<HashTables>();
    limits        = std::make_unique<Limits>(*hashtables);
    marketbook    = std::make_unique<MarketBook>(*hashtables);
    order_manager = std::make_unique<OrderManager>(
                        parser_to_store,
                        store_to_strategy,
                        store_to_strategy_free_slot,
                        store_to_risk,
                        store_to_db,
                        *marketbook,
                        *hashtables
    );
    risk = std::make_unique<RiskEngine>(
                        store_to_risk,
                        strategy_to_risk,
                        risk_to_strategy,
                        risk_to_builder,
                        *hashtables,
                        *marketbook,
                        *limits,
                        *order_manager
    );

    parser_to_store.push(MessageWithVenue<MessageTypes_t>{NASDAQ::ITCHMessage{&test_data_ordMngr::nasdaq_dir}, Venue::NASDAQ});
    order_manager->store();
    parser_to_store.push(MessageWithVenue<MessageTypes_t>{BIST::ITCHMessage{&test_data_ordMngr::bist_dir}, Venue::BIST});
    order_manager->store();

    auto* symmeta = order_manager->get_symbolmeta(Venue::BIST, 3);
    auto* ticksize_entry = new TickSizeEntry(0, 100'000'000, 10);
    symmeta->tick_size_table[0].store(ticksize_entry, std::memory_order_relaxed);

    auto& symRisk = risk->symbolrisks_[0][0];
    symRisk.best_bid.store(10000, std::memory_order_relaxed);
    symRisk.best_ask.store(10000, std::memory_order_relaxed);

    consumer = std::thread([&]
    {
        pin_to_cpu(0);
        Order* order;
        while (!stop.load(std::memory_order_acquire))
            if (!risk_to_builder.pop(order)) _mm_pause();
    });

    consumer2 = std::thread([&]
    {
        pin_to_cpu(4);
        OrderWithRejectReason orderWRR;
        while (!stop2.load(std::memory_order_acquire))
            if (!risk_to_strategy.pop(orderWRR)) _mm_pause();
    });

    auto run = [&]()
    {
        strategy_to_risk.push(test_data_risk::o1);

        asm volatile("" ::: "memory");

        bool processing = true;
        while (processing)
            processing = risk->check_risk();

        reset_accountrisk(risk->accountrisks_[0]);
        reset_symbolrisk(risk->symbolrisks_[0][0]);
        risk->orderrisks_[0].clear();
    };

    constexpr int WARMUP = 10'000;
    for (int i = 0; i < WARMUP; i++)
        run();

    constexpr int N = 5'000'000;
    for (int i = 0; i < N; i++)
        run();

    stop.store(true, std::memory_order_release);
    stop2.store(true, std::memory_order_release);
    consumer.join();
    consumer2.join();

    risk.reset();
    order_manager.reset();
    marketbook.reset();
    limits.reset();
    hashtables.reset();

    return 0;
}