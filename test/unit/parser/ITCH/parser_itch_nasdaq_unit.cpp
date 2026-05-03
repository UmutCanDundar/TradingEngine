#include <gtest/gtest.h>

#include "Parser_ITCH_NASDAQ.h"
#include "dataset.h"

TEST(ITCHNASDAQParserTest, StockDirectory)
{
    Parser_ITCH_NASDAQ parser;
    NASDAQ::ITCHMessage variant_msg = parser.parse(
        reinterpret_cast<const char*>(test_data::itch_nasdaq_pkt1.data())
    );

    auto* msg = std::get<NASDAQ::ITCHStockDirectoryMessage*>(variant_msg);
    
    ASSERT_NE(msg, nullptr);
    ASSERT_EQ(msg->message_type, 'R');
    ASSERT_EQ(msg->stock_locate, 1);
    ASSERT_EQ(msg->tracking_number, 2);
    ASSERT_EQ(msg->timestamp, 48);
    ASSERT_EQ(msg->round_lot_size, 100);
    ASSERT_EQ(msg->market_category, 'Q');
    ASSERT_EQ(msg->financial_status_indicator, 'N');
    ASSERT_EQ(msg->round_lots_only, 'N');
    ASSERT_EQ(msg->issue_classification, 'C');
    ASSERT_EQ(msg->issue_sub_type[0], 'Z');
    ASSERT_EQ(msg->issue_sub_type[1], 'Z');
    ASSERT_EQ(msg->authenticity, 'P');
    ASSERT_EQ(msg->short_sale_threshold_indicator, 'N');
    ASSERT_EQ(msg->ipo_flag, 'N');
    ASSERT_EQ(msg->luld_reference_price_tier, '1');
    ASSERT_EQ(msg->etp_flag, 'N');
    ASSERT_EQ(msg->etp_leverage_factor, 1);
    ASSERT_EQ(msg->inverse_indicator, 'N');
    ASSERT_EQ(msg->stock[0], 'A');
    ASSERT_EQ(msg->stock[1], 'A');
    ASSERT_EQ(msg->stock[2], 'P');
    ASSERT_EQ(msg->stock[3], 'L');
}

TEST(ITCHNASDAQParserTest, AddOrder)
{
    Parser_ITCH_NASDAQ parser;
    NASDAQ::ITCHMessage variant_msg = parser.parse(
        reinterpret_cast<const char*>(test_data::itch_nasdaq_pkt2.data())
    );
    
    auto* msg = std::get<NASDAQ::ITCHAddOrderMessage*>(variant_msg);

    ASSERT_NE(msg, nullptr);
    ASSERT_EQ(msg->message_type, 'A');
    ASSERT_EQ(msg->stock_locate, 1);
    ASSERT_EQ(msg->tracking_number, 2);
    ASSERT_EQ(msg->timestamp, 100);
    ASSERT_EQ(msg->order_ref, 1);
    ASSERT_EQ(msg->side, 'B');
    ASSERT_EQ(msg->shares, 100);
    ASSERT_EQ(msg->price, 10000);
    ASSERT_EQ(msg->stock[0], 'A');
    ASSERT_EQ(msg->stock[1], 'A');
    ASSERT_EQ(msg->stock[2], 'P');
    ASSERT_EQ(msg->stock[3], 'L');
}

TEST(ITCHNASDAQParserTest, OrderExecuted)
{
    Parser_ITCH_NASDAQ parser;
    NASDAQ::ITCHMessage variant_msg = parser.parse(
        reinterpret_cast<const char*>(test_data::itch_nasdaq_pkt3.data())
    );

    auto* msg = std::get<NASDAQ::ITCHExecutedMessage*>(variant_msg);

    ASSERT_NE(msg, nullptr);
    ASSERT_EQ(msg->message_type, 'E');
    ASSERT_EQ(msg->stock_locate, 1);
    ASSERT_EQ(msg->tracking_number, 3);
    ASSERT_EQ(msg->timestamp, 200);
    ASSERT_EQ(msg->order_ref, 1);
    ASSERT_EQ(msg->executed_shares, 50);
    ASSERT_EQ(msg->match_number, 0x99);
}