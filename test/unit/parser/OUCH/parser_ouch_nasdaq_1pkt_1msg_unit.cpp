#include <gtest/gtest.h>

#include "Parser_OUCH_NASDAQ.h"
#include "dataset_parser.h"

TEST(OUCHNASDAQParserTest, Accepted)
{
    Parser_OUCH_NASDAQ parser;
    auto variant_msg = parser.parse(reinterpret_cast<const char*>(test_data_parser::ouch_nasdaq_pkt1.data()));

    auto* msg = std::get<NASDAQ::OUT::OUCHOrderAcceptedMessage*>(variant_msg);

    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->message_type, 'A');
    EXPECT_EQ(msg->timestamp, 1);
    EXPECT_EQ(msg->user_ref_num, 1);
    EXPECT_EQ(msg->side, 'B');
    EXPECT_EQ(msg->quantity, 1000);
    EXPECT_EQ(std::string(msg->symbol, 8), std::string("AAPL    "));
    EXPECT_EQ(msg->price, 10000);
    EXPECT_EQ(msg->time_in_force, '0');
    EXPECT_EQ(msg->display, 'Y');
    EXPECT_EQ(msg->order_reference_number, 2);
    EXPECT_EQ(msg->capacity, 'A');
    EXPECT_EQ(msg->intermarket_sweep_eligibility, 'N');
    EXPECT_EQ(msg->cross_type, 'N');
    EXPECT_EQ(msg->order_state, 'L');
    EXPECT_EQ(std::string(msg->cl_ord_id, 14), std::string("CLIENT0000001 "));
    EXPECT_EQ(msg->appendage_length, 0);
}

TEST(OUCHNASDAQParserTest, Cancelled)
{
    Parser_OUCH_NASDAQ parser;
    auto variant_msg = parser.parse(reinterpret_cast<const char*>(test_data_parser::ouch_nasdaq_pkt2.data()));

    auto* msg = std::get<NASDAQ::OUT::OUCHOrderCancelledMessage*>(variant_msg);

    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->message_type, 'C');
    EXPECT_EQ(msg->timestamp, 4);
    EXPECT_EQ(msg->user_ref_num, 2);
    EXPECT_EQ(msg->quantity, 500);
    EXPECT_EQ(msg->reason, 'U');
    EXPECT_EQ(msg->appendage_length, 0);
}

TEST(OUCHNASDAQParserTest, Replaced)
{
    Parser_OUCH_NASDAQ parser;
    auto variant_msg = parser.parse(reinterpret_cast<const char*>(test_data_parser::ouch_nasdaq_pkt3.data()));

    auto* msg = std::get<NASDAQ::OUT::OUCHOrderReplacedMessage*>(variant_msg);

    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->message_type, 'U');
    EXPECT_EQ(msg->timestamp, 3);
    EXPECT_EQ(msg->orig_user_ref_num, 1);
    EXPECT_EQ(msg->user_ref_num, 2);
    EXPECT_EQ(msg->side, 'B');
    EXPECT_EQ(msg->quantity, 1000);
    EXPECT_EQ(std::string(msg->symbol, 8), std::string("AAPL    "));
    EXPECT_EQ(msg->price, 12345);
    EXPECT_EQ(msg->time_in_force, '0');
    EXPECT_EQ(msg->display, 'Y');
    EXPECT_EQ(msg->order_reference_number, 2);
    EXPECT_EQ(msg->capacity, 'A');
    EXPECT_EQ(msg->intermarket_sweep_eligibility, 'N');
    EXPECT_EQ(msg->cross_type, 'N');
    EXPECT_EQ(msg->order_state, 'L');
    EXPECT_EQ(std::string(msg->cl_ord_id, 14), std::string("CLIENT0000002 "));
    EXPECT_EQ(msg->appendage_length, 0);
}