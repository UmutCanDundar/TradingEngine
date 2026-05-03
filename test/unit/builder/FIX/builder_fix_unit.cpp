#include <gtest/gtest.h>

#include "SessionManager.h"
#include "Builder_FIX.h"
#include "Order.h"
#include "dataset.h"

#include <memory>
#include <unordered_map>
#include <string_view>
#include <cstring>
#include <iostream>

static constexpr char SOH = '\x01';

std::unordered_map<int, std::string_view> make_tag_map(const char* data, size_t len)
{
    std::unordered_map<int, std::string_view> tags;

    size_t i = 0;
    while (i < len)
    {
        const char* start = data + i;
        const char* eq = static_cast<const char*>(std::memchr(start, '=', len - i));
        if (!eq) break;

        int tag = std::atoi(std::string(start, eq).c_str());

        const char* value_start = eq + 1;
        const char* soh = static_cast<const char*>(std::memchr(value_start, SOH, len - (value_start - data)));

        if (!soh) break;

        tags[tag] = std::string_view(value_start, soh - value_start);

        i = (soh - data) + 1;
    }

    return tags;
}

TEST(FixBuilderTest, FixNewOrder)
{
    Order* order = test_data::fix_new_order;

    auto sess_mngr   = std::make_unique<SessionManager>();
    auto builder_fix = std::make_unique<Builder_FIX>(*sess_mngr);

    uint8_t sess_index = sess_mngr->getSessionIndex(Venue::BIST, Protocol::FIX);    

    Buffer_FIX* buf = builder_fix->build<FIXTypes::NewOrderSingle>(sess_index, *order);

    ASSERT_NE(buf, nullptr);
    ASSERT_GT(buf->len, 0);

    auto tags = make_tag_map(buf->data, buf->len);

    // ---------------- HEADER CHECKS ----------------
    EXPECT_EQ(tags[8], "FIX.4.4"); // BeginString
    EXPECT_EQ(tags[9], "154");     // BodyLength 
    EXPECT_EQ(tags[35], "D");       // NewOrderSingle
    EXPECT_EQ(tags[49], "CLIENT01");   // auth_fix.my_id
    EXPECT_EQ(tags[56], "BIST");   // auth_fix.venue_id
    EXPECT_EQ(tags[34], "1");     // MsgSeqNum
    EXPECT_TRUE(tags.count(52));     // TransactTime
    EXPECT_EQ(tags[52].size(), 21u);

    // ---------------- BODY CHECKS ----------------
    EXPECT_EQ(tags[11], order->client_order_token.data());
    EXPECT_EQ(tags[1],  "ACC001"); // fix_auth.account
    EXPECT_EQ(tags[528], "A");
    const auto symbol = std::string_view{order->symbol.data(), 5};
    EXPECT_EQ(tags[55], symbol);
    EXPECT_EQ(tags[54], std::to_string(static_cast<int>(order->side)));

    const auto ts = builder_fix->epoch_to_transact_time(order->timestamp);
    const auto ts_str = std::string_view{ts.first, ts.second};
    EXPECT_EQ(tags[60], ts_str);
    
    EXPECT_EQ(tags[38], std::to_string(order->quantity));
    EXPECT_EQ(tags[40], std::to_string(static_cast<int>(order->order_type)));
    EXPECT_EQ(tags[44], "12.3456");
    EXPECT_EQ(tags[59], std::to_string(static_cast<int>(order->time_in_force))); 

    // ---------------- CHECKSUM ----------------
    EXPECT_TRUE(tags.count(10));
    EXPECT_EQ(tags[10].size(), 3u);


std::cout << std::endl;
for(size_t i = 0; i < buf->len; i++){
    if (buf->data[i] == '\x01')
        std::cout << '|';
    else
        std::cout << buf->data[i];
}
std::cout << std::endl << "buffer_len: " << buf->len << std::endl << std::endl;
}


