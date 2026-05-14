#include <gtest/gtest.h>

#include "OrderManager.h"
#include "MarketBook.h"
#include "HashTables.h"
#include "Order.h"
#include "dataset_order_manager.h"

#include <memory>

TEST(OrderManagerTest, MixedMessageTraffic)
{
    spscMessageQueue_t parser_to_store;
    spscOrderQueue_t store_to_strategy;
    spscOrderQueue_t store_to_strategy_free_slot;
    spscOrderQueue_t store_to_risk;
    spscDbQueue_t store_to_db;
    auto hashtables    = std::make_unique<HashTables>();
    auto marketbook    = std::make_unique<MarketBook>(*hashtables);
  
    auto order_manager = std::make_unique<OrderManager>(
                                        parser_to_store,
                                        store_to_strategy,
                                        store_to_strategy_free_slot,
                                        store_to_risk,
                                        store_to_db,
                                        *marketbook,
                                        *hashtables        
    );

// BIST FIX order
{
    Order* order = nullptr;
    store_to_strategy_free_slot.pop(order);

    order->price              = test_data_ordMngr::fix_new.price;
    order->quantity           = test_data_ordMngr::fix_new.quantity;
    order->side               = Side::Buy;
    order->venue              = Venue::BIST;
    order->isOurOrder         = true;
    order->protocol           = Protocol::FIX;
    order->instrument_id      = 3;
    order->order_type         = static_cast<OrderType>(test_data_ordMngr::fix_new.ord_type);
    order->symbol             = {"GARAN"};
    std::strncpy(order->client_order_token.data(), test_data_ordMngr::fix_new.cl_ord_id, ORDER_TOKEN_SIZE);
    order->client_order_id    = absl::Hash<std::string_view>{}(test_data_ordMngr::fix_new.cl_ord_id);

    order_manager->add_awaitingAck_order(*order);
}

// BIST OUCH order
{
    Order* order = nullptr;
    store_to_strategy_free_slot.pop(order);

    order->price              = test_data_ordMngr::bist_acc.price;
    order->quantity           = test_data_ordMngr::bist_acc.quantity;
    order->side               = Side::Buy;
    order->venue              = Venue::BIST;
    order->isOurOrder         = true;
    order->protocol           = Protocol::OUCH;
    order->instrument_id      = test_data_ordMngr::bist_acc.order_book_id;
    order->symbol             = {"GARAN"};
    std::strncpy(order->client_order_token.data(), test_data_ordMngr::bist_acc.order_token, sizeof(test_data_ordMngr::bist_acc.order_token));
    order->client_order_id    = absl::Hash<std::string_view>{}(std::string_view{test_data_ordMngr::bist_acc.order_token,14});
    order->real_cl_ord_token_len = 14;

    order_manager->add_awaitingAck_order(*order);
}

// NASDAQ OUCH order
{
    Order* order = nullptr;
    store_to_strategy_free_slot.pop(order);

    order->price              = test_data_ordMngr::nasdaq_acc.price;
    order->quantity           = test_data_ordMngr::nasdaq_acc.quantity;
    order->side               = Side::Buy;
    order->venue              = Venue::NASDAQ;
    order->isOurOrder         = true;
    order->protocol           = Protocol::OUCH;
    order->user_ref_num       = test_data_ordMngr::nasdaq_acc.user_ref_num;
    order->symbol             = {"AAPL"};
    order->client_order_id    = absl::Hash<std::string_view>{}(std::string_view{test_data_ordMngr::nasdaq_acc.cl_ord_id,14});
    order->real_cl_ord_token_len = 13;
    std::strncpy(order->client_order_token.data(), test_data_ordMngr::nasdaq_acc.cl_ord_id, sizeof(test_data_ordMngr::nasdaq_acc.cl_ord_id));


    order_manager->add_awaitingAck_order(*order);
}

// ====================================================
//       v TRAFFIC FOR ORDER MANAGER v 
    
std::array<MessageWithVenue<MessageTypes_t>, 20> ord_manager_traffic = []()
{
    std::array<MessageWithVenue<MessageTypes_t>, 20> arr{};

    // 0 — NASDAQ ITCH StockDirectory (AAPL)
    arr[0] = MessageWithVenue<MessageTypes_t>{NASDAQ::ITCHMessage{&test_data_ordMngr::nasdaq_dir}, Venue::NASDAQ};

    // 1 — BIST ITCH OrderBookDirectory (GARAN)
    arr[1] = MessageWithVenue<MessageTypes_t>{BIST::ITCHMessage{&test_data_ordMngr::bist_dir}, Venue::BIST}; 

    // 2 — FIX New Order (GARAN, buy 500@15000)
    arr[2] = MessageWithVenue<MessageTypes_t>{&test_data_ordMngr::fix_new, Venue::BIST};  /////////////

    // 3 — BIST OUCH OrderAccepted (GARAN, order_id=42)
    arr[3] = MessageWithVenue<MessageTypes_t>{BIST::OUCHMessage{&test_data_ordMngr::bist_acc}, Venue::BIST};  ///////////////

    // 4 — BIST ITCH AddOrder (GARAN, 1000@10000)
    arr[4] = MessageWithVenue<MessageTypes_t>{BIST::ITCHMessage{&test_data_ordMngr::bist_add}, Venue::BIST}; //////////////

    // 5 — NASDAQ OUCH OrderAccepted (AAPL, user_ref=99)
    arr[5] = MessageWithVenue<MessageTypes_t>{NASDAQ::OUCHMessage{&test_data_ordMngr::nasdaq_acc}, Venue::NASDAQ};   //////////////////

    // 6 — NASDAQ ITCH AddOrder (AAPL, 100@10000)
    arr[6] = MessageWithVenue<MessageTypes_t>{NASDAQ::ITCHMessage{&test_data_ordMngr::nasdaq_add}, Venue::NASDAQ};  /////////////

    // 7 — FIX Partial Fill (GARAN, 200 filled)
    arr[7] = MessageWithVenue<MessageTypes_t>{&test_data_ordMngr::fix_partial, Venue::BIST}; ////////////////

    // 8 — BIST ITCH OrderExecuted (GARAN, 60 executed)
    arr[8] = MessageWithVenue<MessageTypes_t>{BIST::ITCHMessage{&test_data_ordMngr::bist_exec}, Venue::BIST}; ////////////////

    // 9 — BIST OUCH OrderExecuted (GARAN, 600 traded)
    arr[9] = MessageWithVenue<MessageTypes_t>{BIST::OUCHMessage{&test_data_ordMngr::bist_execO}, Venue::BIST}; //////////

    // 10 — NASDAQ ITCH OrderExecuted (AAPL, 60 executed)
    arr[10] = MessageWithVenue<MessageTypes_t>{NASDAQ::ITCHMessage{&test_data_ordMngr::nasdaq_exec}, Venue::NASDAQ}; 

    // 11 — NASDAQ OUCH OrderExecuted (AAPL, 600 executed)
    arr[11] = MessageWithVenue<MessageTypes_t>{NASDAQ::OUCHMessage{&test_data_ordMngr::nasdaq_execO}, Venue::NASDAQ};   //////////////

    // 12 — BIST ITCH OrderDelete (GARAN, kalan 40 deleted)
    arr[12] = MessageWithVenue<MessageTypes_t>{BIST::ITCHMessage{&test_data_ordMngr::bist_cancel}, Venue::BIST}; ////////////////

    // 13 — BIST OUCH OrderCancelled (GARAN)
    arr[13] = MessageWithVenue<MessageTypes_t>{BIST::OUCHMessage{&test_data_ordMngr::bist_canO}, Venue::BIST}; /////////////

    // 14 — NASDAQ ITCH OrderDelete (AAPL, remaining 40 deleted)
    arr[14] = MessageWithVenue<MessageTypes_t>{NASDAQ::ITCHMessage{&test_data_ordMngr::nasdaq_delete}, Venue::NASDAQ};

    // 15 — NASDAQ OUCH OrderCancelled (AAPL)
    arr[15] = MessageWithVenue<MessageTypes_t>{NASDAQ::OUCHMessage{&test_data_ordMngr::nasdaq_canO}, Venue::NASDAQ};  //////////////////

    // 16 — FIX Cancel Confirm (GARAN)
    arr[16] = MessageWithVenue<MessageTypes_t>{&test_data_ordMngr::fix_cancel, Venue::BIST}; ///////////////

    // 17 — BIST ITCH AddOrder (GARAN, 1000@1000)
    arr[17] = MessageWithVenue<MessageTypes_t>{BIST::ITCHMessage{&test_data_ordMngr::bist_add_2}, Venue::BIST}; //////////////
    // 18 — BIST ITCH AddOrder (GARAN, 1000@10000 S)
    arr[18] = MessageWithVenue<MessageTypes_t>{BIST::ITCHMessage{&test_data_ordMngr::bist_add_3}, Venue::BIST}; //////////////
    // 19 — BIST ITCH AddOrder (GARAN, 1000@1000 S)
    arr[19] = MessageWithVenue<MessageTypes_t>{BIST::ITCHMessage{&test_data_ordMngr::bist_add_4}, Venue::BIST}; //////////////    
 
    return arr;
}();
 
 
// SYMBOLMETA INITIALIZING
    parser_to_store.push(ord_manager_traffic[0]);
    order_manager->store();
    
    parser_to_store.push(ord_manager_traffic[1]);
    order_manager->store();
    
//      ^ TRAFFIC FOR ORDER MANAGER ^
// ====================================================
  
    Order* order = nullptr; 

// ─────────────────────────────────────────────────────────────────────────────
// arr[2] → FIX New Order (GARAN, buy 500@15000)
// ─────────────────────────────────────────────────────────────────────────────
    parser_to_store.push(ord_manager_traffic[2]);
    order_manager->store();

    store_to_risk.pop(order);
    ASSERT_NE(order, nullptr);

    EXPECT_EQ(order->price,              15000);
    EXPECT_EQ(order->quantity,           500);
    EXPECT_EQ(order->filled_quantity,    0);
    EXPECT_EQ(order->remaining_quantity, 500);
    EXPECT_EQ(order->last_exec_quantity, 0);
    EXPECT_EQ(order->replaced_quantity,  0);
    EXPECT_EQ(order->instrument_id,      3);
    EXPECT_EQ(order->side,               Side::Buy);
    EXPECT_EQ(order->status,             Status::New);
    EXPECT_EQ(order->venue,              Venue::BIST);
    EXPECT_EQ(order->isOurOrder,         true);
    EXPECT_EQ(order->protocol,           Protocol::FIX);
    EXPECT_EQ(order->order_type,         OrderType::Limit);
    EXPECT_EQ(order->time_in_force,      TimeInForce::Unknown);
    EXPECT_EQ(order->order_id,           absl::Hash<std::string_view>{}(test_data_ordMngr::fix_new.order_id));
    EXPECT_EQ(order->exec_type,          '0');
    EXPECT_EQ(order->syncState,          SyncState::NewSeen);
    EXPECT_EQ(order->cancelled_count,    0);
    EXPECT_EQ(std::string(order->symbol.data()),             "GARAN");
    EXPECT_EQ(std::string(order->client_order_token.data()), "F1");
    EXPECT_EQ(std::string(order->fix_org_order_id.data()),   "O1");


// ─────────────────────────────────────────────────────────────────────────────
// arr[3] → BIST OUCH OrderAccepted (GARAN)
// ─────────────────────────────────────────────────────────────────────────────
    parser_to_store.push(ord_manager_traffic[3]);
    order_manager->store();
    
    store_to_risk.pop(order);
    ASSERT_NE(order, nullptr);

    EXPECT_EQ(order->price,              10000);
    EXPECT_EQ(order->quantity,           100);
    EXPECT_EQ(order->filled_quantity,    0);
    EXPECT_EQ(order->remaining_quantity, 100);
    EXPECT_EQ(order->last_exec_quantity, 0);
    EXPECT_EQ(order->replaced_quantity,  0);
    EXPECT_EQ(order->instrument_id,      3);
    EXPECT_EQ(order->side,               Side::Buy);
    EXPECT_EQ(order->status,             Status::New);
    EXPECT_EQ(order->venue,              Venue::BIST);
    EXPECT_EQ(order->isOurOrder,         true);
    EXPECT_EQ(order->protocol,           Protocol::OUCH);
    EXPECT_EQ(order->order_type,         OrderType::Unknown);
    EXPECT_EQ(order->time_in_force,      TimeInForce::Unknown);
    EXPECT_EQ(order->order_id,           42);
    EXPECT_EQ(order->exec_type,          0);
    EXPECT_EQ(order->syncState,          SyncState::WaitingNew);
    EXPECT_EQ(order->cancelled_count,    0);
    EXPECT_EQ(std::string(order->symbol.data()),             "GARAN");
    EXPECT_EQ(std::string(order->client_order_token.data()), "TOKEN000000042");
    EXPECT_EQ(std::string(order->fix_org_order_id.data()),   "");


// ─────────────────────────────────────────────────────────────────────────────
// arr[4] → BIST ITCH AddOrder (GARAN)
// ─────────────────────────────────────────────────────────────────────────────
    parser_to_store.push(ord_manager_traffic[4]);
    order_manager->store();
    
    store_to_risk.pop(order);
    ASSERT_NE(order, nullptr);

    EXPECT_EQ(order->price,              10000);
    EXPECT_EQ(order->quantity,           1000);
    EXPECT_EQ(order->filled_quantity,    0);
    EXPECT_EQ(order->remaining_quantity, 1000);
    EXPECT_EQ(order->last_exec_quantity, 0);
    EXPECT_EQ(order->replaced_quantity,  0);
    EXPECT_EQ(order->side,               Side::Buy);
    EXPECT_EQ(order->status,             Status::New);
    EXPECT_EQ(order->venue,              Venue::BIST);
    EXPECT_EQ(order->isOurOrder,         false);
    EXPECT_EQ(order->protocol,           Protocol::ITCH);
    EXPECT_EQ(order->order_type,         OrderType::Market);
    EXPECT_EQ(order->time_in_force,      TimeInForce::Unknown);
    EXPECT_EQ(order->order_id,           42);
    EXPECT_EQ(order->timestamp,          100);
    EXPECT_EQ(order->last_update_time,   100);
    EXPECT_EQ(order->cancelled_count,    0);
    EXPECT_EQ(std::string(order->symbol.data()),             "GARAN");
    EXPECT_EQ(std::string(order->client_order_token.data()), "");
    EXPECT_EQ(std::string(order->fix_org_order_id.data()),   "");

    auto symbook = marketbook->get_symBook(*order);
    EXPECT_EQ(marketbook->best_bid(*symbook),       10000);
    EXPECT_EQ(marketbook->best_bid_qty(*symbook),    1000);
    EXPECT_EQ(marketbook->best_ask(*symbook),           0);
    EXPECT_EQ(marketbook->best_ask_qty(*symbook),       0);

// ─────────────────────────────────────────────────────────────────────────────
// arr[5] → NASDAQ OUCH OrderAccepted (AAPL)
// ─────────────────────────────────────────────────────────────────────────────
    parser_to_store.push(ord_manager_traffic[5]);
    order_manager->store();
    
    store_to_risk.pop(order);
    ASSERT_NE(order, nullptr);

    EXPECT_EQ(order->price,              10000);
    EXPECT_EQ(order->quantity,           1000);
    EXPECT_EQ(order->filled_quantity,    0);
    EXPECT_EQ(order->remaining_quantity, 1000);
    EXPECT_EQ(order->last_exec_quantity, 0);
    EXPECT_EQ(order->replaced_quantity,  0);
    EXPECT_EQ(order->user_ref_num,       99);
    EXPECT_EQ(order->side,               Side::Buy);
    EXPECT_EQ(order->status,             Status::New);
    EXPECT_EQ(order->venue,              Venue::NASDAQ);
    EXPECT_EQ(order->isOurOrder,         true);
    EXPECT_EQ(order->protocol,           Protocol::OUCH);
    EXPECT_EQ(order->order_type,         OrderType::Unknown);
    EXPECT_EQ(order->time_in_force,      TimeInForce::Unknown);
    EXPECT_EQ(order->order_id,           99);
    EXPECT_EQ(order->exec_type,          0);
    EXPECT_EQ(order->syncState,          SyncState::WaitingNew);
    EXPECT_EQ(order->cancelled_count,    0);
    EXPECT_EQ(std::string(order->symbol.data()),             "AAPL");
    EXPECT_EQ(std::string(order->client_order_token.data()), "CLIENT0000001 ");

// ─────────────────────────────────────────────────────────────────────────────
// arr[6] → NASDAQ ITCH AddOrder (AAPL)
// ─────────────────────────────────────────────────────────────────────────────
    parser_to_store.push(ord_manager_traffic[6]);
    order_manager->store();
    
    store_to_risk.pop(order);
    ASSERT_NE(order, nullptr);

    EXPECT_EQ(order->price,              10000);
    EXPECT_EQ(order->quantity,           100);
    EXPECT_EQ(order->filled_quantity,    0);
    EXPECT_EQ(order->remaining_quantity, 100);
    EXPECT_EQ(order->last_exec_quantity, 0);
    EXPECT_EQ(order->replaced_quantity,  0);
    EXPECT_EQ(order->user_ref_num,       0);
    EXPECT_EQ(order->side,               Side::Buy);
    EXPECT_EQ(order->status,             Status::New);
    EXPECT_EQ(order->venue,              Venue::NASDAQ);
    EXPECT_EQ(order->isOurOrder,         false);
    EXPECT_EQ(order->protocol,           Protocol::ITCH);
    EXPECT_EQ(order->order_type,         OrderType::Market);
    EXPECT_EQ(order->time_in_force,      TimeInForce::Unknown);
    EXPECT_EQ(order->order_id,           99);
    EXPECT_EQ(order->timestamp,          100);
    EXPECT_EQ(order->last_update_time,   100);
    EXPECT_EQ(order->exec_type,          0);
    EXPECT_EQ(order->syncState,          SyncState::WaitingNew);
    EXPECT_EQ(order->cancelled_count,    0);
    EXPECT_EQ(std::string(order->symbol.data()),             "AAPL");
    EXPECT_EQ(std::string(order->client_order_token.data()), "");
    EXPECT_EQ(std::string(order->fix_org_order_id.data()),   "");


// ─────────────────────────────────────────────────────────────────────────────
// arr[7] → FIX Partial Fill (GARAN)
// ─────────────────────────────────────────────────────────────────────────────
    parser_to_store.push(ord_manager_traffic[7]);
    order_manager->store();
    
    store_to_risk.pop(order);
    ASSERT_NE(order, nullptr);

    EXPECT_EQ(order->price,              15000);
    EXPECT_EQ(order->quantity,           500);
    EXPECT_EQ(order->filled_quantity,    200);
    EXPECT_EQ(order->remaining_quantity, 300);
    EXPECT_EQ(order->last_exec_quantity, 200);
    EXPECT_EQ(order->replaced_quantity,  0);
    EXPECT_EQ(order->instrument_id,      3);    
    EXPECT_EQ(order->side,               Side::Buy);
    EXPECT_EQ(order->status,             Status::Partial);
    EXPECT_EQ(order->venue,              Venue::BIST);
    EXPECT_EQ(order->isOurOrder,         true);
    EXPECT_EQ(order->protocol,           Protocol::FIX);
    EXPECT_EQ(order->order_type,         OrderType::Limit);
    EXPECT_EQ(order->time_in_force,      TimeInForce::Unknown);
    EXPECT_EQ(order->order_id,           absl::Hash<std::string_view>{}(test_data_ordMngr::fix_partial.order_id));
    EXPECT_EQ(order->exec_type,          'F');
    EXPECT_EQ(order->syncState,          SyncState::NewSeen);
    EXPECT_EQ(order->cancelled_count,    0);
    EXPECT_EQ(std::string(order->symbol.data()),             "GARAN");
    EXPECT_EQ(std::string(order->client_order_token.data()), "F1");
    EXPECT_EQ(std::string(order->fix_org_order_id.data()),   "O1");


// ─────────────────────────────────────────────────────────────────────────────
// arr[8] → BIST ITCH Executed (GARAN)
// ─────────────────────────────────────────────────────────────────────────────
    parser_to_store.push(ord_manager_traffic[8]);
    order_manager->store();
    
    store_to_risk.pop(order);
    ASSERT_NE(order, nullptr);

    EXPECT_EQ(order->price,              10000);
    EXPECT_EQ(order->quantity,           1000);
    EXPECT_EQ(order->filled_quantity,    600);
    EXPECT_EQ(order->remaining_quantity, 400);
    EXPECT_EQ(order->last_exec_quantity, 600);
    EXPECT_EQ(order->replaced_quantity,  0);
    EXPECT_EQ(order->user_ref_num,       0);
    EXPECT_EQ(order->side,               Side::Buy);
    EXPECT_EQ(order->status,             Status::Partial);
    EXPECT_EQ(order->venue,              Venue::BIST);
    EXPECT_EQ(order->isOurOrder,         false);
    EXPECT_EQ(order->protocol,           Protocol::ITCH);
    EXPECT_EQ(order->order_type,         OrderType::Market);
    EXPECT_EQ(order->time_in_force,      TimeInForce::Unknown);
    EXPECT_EQ(order->order_id,           42);
    EXPECT_EQ(order->timestamp,          100);
    EXPECT_EQ(order->last_update_time,   200);
    EXPECT_EQ(order->exec_type,          0);
    EXPECT_EQ(order->syncState,          SyncState::WaitingNew);
    EXPECT_EQ(order->cancelled_count,    0);
    EXPECT_EQ(std::string(order->symbol.data()),             "GARAN");
    EXPECT_EQ(std::string(order->client_order_token.data()), "");
    EXPECT_EQ(std::string(order->fix_org_order_id.data()),   "");

    symbook = marketbook->get_symBook(*order);
    EXPECT_EQ(marketbook->best_bid(*symbook),       10000);
    EXPECT_EQ(marketbook->best_bid_qty(*symbook),     400);
    EXPECT_EQ(marketbook->best_ask(*symbook),           0);
    EXPECT_EQ(marketbook->best_ask_qty(*symbook),       0);


// ─────────────────────────────────────────────────────────────────────────────
// arr[9] → BIST OUCH Executed (GARAN)
// ─────────────────────────────────────────────────────────────────────────────
    parser_to_store.push(ord_manager_traffic[9]);
    order_manager->store();

    store_to_risk.pop(order);
    ASSERT_NE(order, nullptr);

    EXPECT_EQ(order->price,              10000);
    EXPECT_EQ(order->quantity,           100);
    EXPECT_EQ(order->filled_quantity,    60);
    EXPECT_EQ(order->remaining_quantity, 40);
    EXPECT_EQ(order->last_exec_quantity, 60);
    EXPECT_EQ(order->replaced_quantity,  0);
    EXPECT_EQ(order->instrument_id,      3);
    EXPECT_EQ(order->side,               Side::Buy);
    EXPECT_EQ(order->status,             Status::Partial);
    EXPECT_EQ(order->venue,              Venue::BIST);
    EXPECT_EQ(order->isOurOrder,         true);
    EXPECT_EQ(order->protocol,           Protocol::OUCH);
    EXPECT_EQ(order->order_type,         OrderType::Unknown);
    EXPECT_EQ(order->time_in_force,      TimeInForce::Unknown);
    EXPECT_EQ(order->order_id,           42);
    EXPECT_EQ(order->exec_type,          0);
    EXPECT_EQ(order->syncState,          SyncState::WaitingNew);
    EXPECT_EQ(order->cancelled_count,    0);
    EXPECT_EQ(std::string(order->symbol.data()),             "GARAN");
    EXPECT_EQ(std::string(order->client_order_token.data()), "TOKEN000000042");
    EXPECT_EQ(std::string(order->fix_org_order_id.data()),   "");


// ─────────────────────────────────────────────────────────────────────────────
// arr[10] → NASDAQ ITCH Executed (AAPL)
// ─────────────────────────────────────────────────────────────────────────────
    parser_to_store.push(ord_manager_traffic[10]);
    order_manager->store();

    store_to_risk.pop(order);
    ASSERT_NE(order, nullptr);

    EXPECT_EQ(order->price,              10000);
    EXPECT_EQ(order->quantity,           100);
    EXPECT_EQ(order->filled_quantity,    60);
    EXPECT_EQ(order->remaining_quantity, 40);
    EXPECT_EQ(order->last_exec_quantity, 60);
    EXPECT_EQ(order->replaced_quantity,  0);
    EXPECT_EQ(order->user_ref_num,       0);
    EXPECT_EQ(order->side,               Side::Buy);
    EXPECT_EQ(order->status,             Status::Partial);
    EXPECT_EQ(order->venue,              Venue::NASDAQ);
    EXPECT_EQ(order->isOurOrder,         false);
    EXPECT_EQ(order->protocol,           Protocol::ITCH);
    EXPECT_EQ(order->order_type,         OrderType::Market);
    EXPECT_EQ(order->time_in_force,      TimeInForce::Unknown);
    EXPECT_EQ(order->order_id,           99);
    EXPECT_EQ(order->timestamp,          100);
    EXPECT_EQ(order->last_update_time,   200);
    EXPECT_EQ(order->exec_type,          0);
    EXPECT_EQ(order->syncState,          SyncState::WaitingNew);
    EXPECT_EQ(order->cancelled_count,    0);
    EXPECT_EQ(std::string(order->symbol.data()),             "AAPL");
    EXPECT_EQ(std::string(order->client_order_token.data()), "");
    EXPECT_EQ(std::string(order->fix_org_order_id.data()),   "");


// ─────────────────────────────────────────────────────────────────────────────
// arr[11] → NASDAQ OUCH Executed (AAPL)
// ─────────────────────────────────────────────────────────────────────────────
    parser_to_store.push(ord_manager_traffic[11]);
    order_manager->store();

    store_to_risk.pop(order);
    ASSERT_NE(order, nullptr);

    EXPECT_EQ(order->price,              10000);
    EXPECT_EQ(order->quantity,           1000);
    EXPECT_EQ(order->filled_quantity,    600);
    EXPECT_EQ(order->remaining_quantity, 400);
    EXPECT_EQ(order->last_exec_quantity, 600);
    EXPECT_EQ(order->replaced_quantity,  0);
    EXPECT_EQ(order->user_ref_num,       99);
    EXPECT_EQ(order->side,               Side::Buy);
    EXPECT_EQ(order->status,             Status::Partial);
    EXPECT_EQ(order->venue,              Venue::NASDAQ);
    EXPECT_EQ(order->isOurOrder,         true);
    EXPECT_EQ(order->protocol,           Protocol::OUCH);
    EXPECT_EQ(order->order_type,         OrderType::Unknown);
    EXPECT_EQ(order->time_in_force,      TimeInForce::Unknown);
    EXPECT_EQ(order->order_id,           99);
    EXPECT_EQ(order->exec_type,          0);
    EXPECT_EQ(order->syncState,          SyncState::WaitingNew);
    EXPECT_EQ(order->cancelled_count,    0);
    EXPECT_EQ(std::string(order->symbol.data()),             "AAPL");
    EXPECT_EQ(std::string(order->client_order_token.data()), "CLIENT0000001 ");


// ─────────────────────────────────────────────────────────────────────────────
// arr[12] → BIST ITCH Delete (GARAN)
// ─────────────────────────────────────────────────────────────────────────────
    parser_to_store.push(ord_manager_traffic[12]);
    order_manager->store();

    store_to_risk.pop(order);
    ASSERT_NE(order, nullptr);

    EXPECT_EQ(order->price,              10000);
    EXPECT_EQ(order->quantity,           1000);
    EXPECT_EQ(order->filled_quantity,    600);
    EXPECT_EQ(order->remaining_quantity, 400);
    EXPECT_EQ(order->last_exec_quantity, 600);
    EXPECT_EQ(order->replaced_quantity,  0);
    EXPECT_EQ(order->user_ref_num,       0);
    EXPECT_EQ(order->side,               Side::Buy);
    EXPECT_EQ(order->status,             Status::Deleted);
    EXPECT_EQ(order->venue,              Venue::BIST);
    EXPECT_EQ(order->isOurOrder,         false);
    EXPECT_EQ(order->protocol,           Protocol::ITCH);
    EXPECT_EQ(order->order_type,         OrderType::Market);
    EXPECT_EQ(order->time_in_force,      TimeInForce::Unknown);
    EXPECT_EQ(order->order_id,           42);
    EXPECT_EQ(order->timestamp,          100);
    EXPECT_EQ(order->last_update_time,   300);
    EXPECT_EQ(order->exec_type,          0);
    EXPECT_EQ(order->syncState,          SyncState::WaitingNew);
    EXPECT_EQ(order->cancelled_count,    1);
    EXPECT_EQ(std::string(order->symbol.data()),             "GARAN");
    EXPECT_EQ(std::string(order->client_order_token.data()), "");
    EXPECT_EQ(std::string(order->fix_org_order_id.data()),   "");

    symbook = marketbook->get_symBook(*order);
    EXPECT_EQ(marketbook->best_bid(*symbook),           0);
    EXPECT_EQ(marketbook->best_bid_qty(*symbook),       0);
    EXPECT_EQ(marketbook->best_ask(*symbook),           0);
    EXPECT_EQ(marketbook->best_ask_qty(*symbook),       0);

// ─────────────────────────────────────────────────────────────────────────────
// arr[13] → BIST OUCH Cancelled (GARAN)
// ─────────────────────────────────────────────────────────────────────────────
    parser_to_store.push(ord_manager_traffic[13]);
    order_manager->store();

    store_to_risk.pop(order);
    ASSERT_NE(order, nullptr);

    EXPECT_EQ(order->price,              10000);
    EXPECT_EQ(order->quantity,           100);
    EXPECT_EQ(order->filled_quantity,    60);
    EXPECT_EQ(order->remaining_quantity, 0);
    EXPECT_EQ(order->last_exec_quantity, 60);
    EXPECT_EQ(order->replaced_quantity,  -40);
    EXPECT_EQ(order->instrument_id,      3);
    EXPECT_EQ(order->side,               Side::Buy);
    EXPECT_EQ(order->status,             Status::Deleted);
    EXPECT_EQ(order->venue,              Venue::BIST);
    EXPECT_EQ(order->isOurOrder,         true);
    EXPECT_EQ(order->protocol,           Protocol::OUCH);
    EXPECT_EQ(order->order_type,         OrderType::Unknown);
    EXPECT_EQ(order->time_in_force,      TimeInForce::Unknown);
    EXPECT_EQ(order->order_id,           42);
    EXPECT_EQ(order->exec_type,          0);
    EXPECT_EQ(order->syncState,          SyncState::WaitingNew);
    EXPECT_EQ(order->cancelled_count,    1);
    EXPECT_EQ(std::string(order->symbol.data()),             "GARAN");
    EXPECT_EQ(std::string(order->client_order_token.data()), "TOKEN000000042");
    EXPECT_EQ(std::string(order->fix_org_order_id.data()),   "");


// ─────────────────────────────────────────────────────────────────────────────
// arr[14] → NASDAQ ITCH Delete (AAPL)
// ─────────────────────────────────────────────────────────────────────────────
    parser_to_store.push(ord_manager_traffic[14]);
    order_manager->store();

    store_to_risk.pop(order);
    ASSERT_NE(order, nullptr);

    EXPECT_EQ(order->price,              10000);
    EXPECT_EQ(order->quantity,           100);
    EXPECT_EQ(order->filled_quantity,    60);
    EXPECT_EQ(order->remaining_quantity, 40);
    EXPECT_EQ(order->last_exec_quantity, 60);
    EXPECT_EQ(order->replaced_quantity,  0);
    EXPECT_EQ(order->user_ref_num,       0);
    EXPECT_EQ(order->side,               Side::Buy);
    EXPECT_EQ(order->status,             Status::Deleted);
    EXPECT_EQ(order->venue,              Venue::NASDAQ);
    EXPECT_EQ(order->isOurOrder,         false);
    EXPECT_EQ(order->protocol,           Protocol::ITCH);
    EXPECT_EQ(order->order_type,         OrderType::Market);
    EXPECT_EQ(order->time_in_force,      TimeInForce::Unknown);
    EXPECT_EQ(order->order_id,           99);
    EXPECT_EQ(order->timestamp,          100);
    EXPECT_EQ(order->last_update_time,   300);
    EXPECT_EQ(order->exec_type,          0);
    EXPECT_EQ(order->syncState,          SyncState::WaitingNew);
    EXPECT_EQ(order->cancelled_count,    1);
    EXPECT_EQ(std::string(order->symbol.data()),             "AAPL");
    EXPECT_EQ(std::string(order->client_order_token.data()), "");
    EXPECT_EQ(std::string(order->fix_org_order_id.data()),   "");


// ─────────────────────────────────────────────────────────────────────────────
// arr[15] → NASDAQ OUCH Cancelled (AAPL)
// ─────────────────────────────────────────────────────────────────────────────
    parser_to_store.push(ord_manager_traffic[15]);
    order_manager->store();

    store_to_risk.pop(order);
    ASSERT_NE(order, nullptr);

    EXPECT_EQ(order->price,              10000);
    EXPECT_EQ(order->quantity,           1000);
    EXPECT_EQ(order->filled_quantity,    600);
    EXPECT_EQ(order->remaining_quantity, 0);
    EXPECT_EQ(order->last_exec_quantity, 600);
    EXPECT_EQ(order->replaced_quantity,  -400);
    EXPECT_EQ(order->user_ref_num,       99);
    EXPECT_EQ(order->side,               Side::Buy);
    EXPECT_EQ(order->status,             Status::Cancelled);
    EXPECT_EQ(order->venue,              Venue::NASDAQ);
    EXPECT_EQ(order->isOurOrder,         true);
    EXPECT_EQ(order->protocol,           Protocol::OUCH);
    EXPECT_EQ(order->order_type,         OrderType::Unknown);
    EXPECT_EQ(order->time_in_force,      TimeInForce::Unknown);
    EXPECT_EQ(order->order_id,           99);
    EXPECT_EQ(order->exec_type,          0);
    EXPECT_EQ(order->syncState,          SyncState::WaitingNew);
    EXPECT_EQ(order->cancelled_count,    1);
    EXPECT_EQ(std::string(order->symbol.data()),             "AAPL");
    EXPECT_EQ(std::string(order->client_order_token.data()), "CLIENT0000001 ");

// ─────────────────────────────────────────────────────────────────────────────
// arr[16] → FIX Cancel Confirm (GARAN)
// ─────────────────────────────────────────────────────────────────────────────
    parser_to_store.push(ord_manager_traffic[16]);
    order_manager->store();

    store_to_risk.pop(order);
    ASSERT_NE(order, nullptr);

    EXPECT_EQ(order->price,              15000);
    EXPECT_EQ(order->quantity,           500);
    EXPECT_EQ(order->filled_quantity,    200);
    EXPECT_EQ(order->remaining_quantity, 0);
    EXPECT_EQ(order->last_exec_quantity, 200);
    EXPECT_EQ(order->replaced_quantity,  -300);
    EXPECT_EQ(order->instrument_id,      3);
    EXPECT_EQ(order->side,               Side::Buy);
    EXPECT_EQ(order->status,             Status::Deleted);
    EXPECT_EQ(order->venue,              Venue::BIST);
    EXPECT_EQ(order->isOurOrder,         true);
    EXPECT_EQ(order->protocol,           Protocol::FIX);
    EXPECT_EQ(order->order_type,         OrderType::Limit);
    EXPECT_EQ(order->time_in_force,      TimeInForce::Unknown);
    EXPECT_EQ(order->order_id,           absl::Hash<std::string_view>{}(test_data_ordMngr::fix_partial.order_id));
    EXPECT_EQ(order->exec_type,          '4');
    EXPECT_EQ(order->syncState,          SyncState::NewSeen);
    EXPECT_EQ(order->cancelled_count,    1);
    EXPECT_EQ(std::string(order->symbol.data()),             "GARAN");
    EXPECT_EQ(std::string(order->client_order_token.data()), "F1");
    EXPECT_EQ(std::string(order->fix_org_order_id.data()),   "O1");


// ─────────────────────────────────────────────────────────────────────────────
// arr[17] → MarketBook Test after a ITCH Messages
// ─────────────────────────────────────────────────────────────────────────────
    parser_to_store.push(ord_manager_traffic[17]);
    order_manager->store();
    store_to_risk.pop(order);
    symbook = marketbook->get_symBook(*order);

    EXPECT_EQ(marketbook->best_bid(*symbook),         100);
    EXPECT_EQ(marketbook->best_bid_qty(*symbook),     100);
    EXPECT_EQ(marketbook->best_ask(*symbook),           0);
    EXPECT_EQ(marketbook->best_ask_qty(*symbook),       0);

// ─────────────────────────────────────────────────────────────────────────────
// arr[18] → MarketBook Test after a ITCH Messages
// ─────────────────────────────────────────────────────────────────────────────
    parser_to_store.push(ord_manager_traffic[18]);
    order_manager->store();
    store_to_risk.pop(order);
    symbook = marketbook->get_symBook(*order);

    EXPECT_EQ(marketbook->best_bid(*symbook),         1000);
    EXPECT_EQ(marketbook->best_bid_qty(*symbook),     1000);
    EXPECT_EQ(marketbook->best_ask(*symbook),           0);
    EXPECT_EQ(marketbook->best_ask_qty(*symbook),       0);

// ─────────────────────────────────────────────────────────────────────────────
// arr[19] → MarketBook Test after a ITCH Messages
// ─────────────────────────────────────────────────────────────────────────────

    parser_to_store.push(ord_manager_traffic[19]);
    order_manager->store();
    store_to_risk.pop(order);
    symbook = marketbook->get_symBook(*order);

    EXPECT_EQ(marketbook->best_bid(*symbook),        1000);
    EXPECT_EQ(marketbook->best_bid_qty(*symbook),    1000);
    EXPECT_EQ(marketbook->best_ask(*symbook),        1000);
    EXPECT_EQ(marketbook->best_ask_qty(*symbook),    1000);

//==============================================================================
    
}

