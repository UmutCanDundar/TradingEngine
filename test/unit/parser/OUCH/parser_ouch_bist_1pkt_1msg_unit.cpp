
#include <gtest/gtest.h>

#include "Parser_OUCH_BIST.h"
#include "dataset_parser.h"


TEST(OUCHBISTParserTest, Accepted)
{
    Parser_OUCH_BIST parser;
    auto variant_msg = parser.parse(
        reinterpret_cast<const char*>(test_data_parser::ouch_bist_pkt1.data()-3)
    );

    auto* msg = std::get<BIST::RX::OUCHOrderAcceptedMessage*>(variant_msg);
    
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->message_type, 'A');
    EXPECT_EQ(msg->timestamp, 1);
    EXPECT_EQ(msg->order_book_id, 2);
    EXPECT_EQ(msg->side, 'B');
    EXPECT_EQ(msg->order_id, 1);
    EXPECT_EQ(msg->quantity, 2000);
    EXPECT_EQ(msg->price, 10000);
    EXPECT_EQ(std::string(msg->order_token, 14), "TOKEN000000002");
    // default
    EXPECT_EQ(msg->time_in_force, 0);
    EXPECT_EQ(msg->open_close, 0);
    EXPECT_EQ(msg->order_state, 1);
    EXPECT_EQ(msg->client_category, 0);
    EXPECT_EQ(msg->offhours, 0);
    EXPECT_EQ(msg->smp_level, 0);
    EXPECT_EQ(msg->smp_method, 0);
    EXPECT_EQ(msg->pre_trade_qty, 0);
    EXPECT_EQ(msg->display_qty, 0);
    EXPECT_EQ(std::string(msg->client_account, 16), std::string(16, '\0'));
    EXPECT_EQ(std::string(msg->customer_info, 15), std::string(15, '\0'));
    EXPECT_EQ(std::string(msg->exchange_info, 32), std::string(32, '\0'));
    EXPECT_EQ(std::string(msg->smp_ID, 3), std::string(3, '\0'));
}

TEST(OUCHBISTParserTest, Cancelled)
{
    Parser_OUCH_BIST parser;
    auto variant_msg = parser.parse(
        reinterpret_cast<const char*>(test_data_parser::ouch_bist_pkt2.data()-3)
    );

    auto* msg = std::get<BIST::RX::OUCHOrderCancelledMessage*>(variant_msg);

    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->message_type, 'C');
    EXPECT_EQ(msg->timestamp, 2);
    EXPECT_EQ(msg->order_book_id, 2);
    EXPECT_EQ(msg->side, 'B');
    EXPECT_EQ(msg->order_id, 1);
    EXPECT_EQ(std::string(msg->order_token, 14), "TOKEN000000002");
    // default
    EXPECT_EQ(msg->reason, 0);
}

TEST(OUCHBISTParserTest, Executed)
{
    Parser_OUCH_BIST parser;
    auto variant_msg = parser.parse(
        reinterpret_cast<const char*>(test_data_parser::ouch_bist_pkt3.data()-3)
    );

    auto* msg = std::get<BIST::RX::OUCHOrderExecutedMessage*>(variant_msg);
    
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->message_type, 'E');
    EXPECT_EQ(msg->timestamp, 3);
    EXPECT_EQ(msg->order_book_id, 2);
    EXPECT_EQ(msg->traded_quantity, 1000);
    EXPECT_EQ(msg->trade_price, 10000);
    EXPECT_EQ(std::string(msg->order_token, 14), "TOKEN000000002");
    // default
    EXPECT_EQ(msg->client_category, 0);
    EXPECT_EQ(std::string(msg->match_id, 12), std::string(12, '\0'));
    EXPECT_EQ(std::string(msg->reserved, 16), std::string(16, '\0'));
}