#include <gtest/gtest.h>

#include "Parser_ITCH_BIST.h"
#include "dataset_parser.h"

TEST(ITCHBISTParserTest, OrderBookDirectory)
{
    Parser_ITCH_BIST parser;
    BIST::ITCHMessage variant_msg = parser.parse(
        reinterpret_cast<const char*>(test_data_parser::itch_bist_pkt1.data())
    );

    auto* msg = std::get<BIST::ITCHOrderBookDirectoryMessage*>(variant_msg);

    ASSERT_NE(msg, nullptr);
    ASSERT_EQ(msg->message_type, 'R');
    ASSERT_EQ(msg->timestamp_ns, 48);
    ASSERT_EQ(msg->order_book_id, 3);
    ASSERT_EQ(msg->financial_product, 5);
    ASSERT_EQ(msg->decimals_in_price, 2);
    ASSERT_EQ(msg->decimals_in_nominal, 2);
    ASSERT_EQ(msg->odd_lot_size, 1);
    ASSERT_EQ(msg->round_lot_size, 1);
    ASSERT_EQ(msg->block_lot_size, 1);
    ASSERT_EQ(msg->nominal_value, 0);
    ASSERT_EQ(msg->number_of_legs, 0);
    ASSERT_EQ(msg->underlying_order_book_id, 0);
    ASSERT_EQ(msg->strike_price, 0);
    ASSERT_EQ(msg->expiration_date, 0);
    ASSERT_EQ(msg->decimals_in_strike_price, 2);
    ASSERT_EQ(msg->put_or_call, 0);
    ASSERT_EQ(msg->ranking_type, 1);
    ASSERT_STREQ(msg->trading_currency, "TRY");
    // symbol: "GARAN" + spaces
    ASSERT_EQ(msg->symbol[0], 'G');
    ASSERT_EQ(msg->symbol[1], 'A');
    ASSERT_EQ(msg->symbol[2], 'R');
    ASSERT_EQ(msg->symbol[3], 'A');
    ASSERT_EQ(msg->symbol[4], 'N');
    // isin
    ASSERT_EQ(msg->isin[0], 'T');
    ASSERT_EQ(msg->isin[1], 'R');
    ASSERT_EQ(msg->isin[2], 'G');
}

TEST(ITCHBISTParserTest, AddOrder)
{
    Parser_ITCH_BIST parser;
    BIST::ITCHMessage variant_msg = parser.parse(
        reinterpret_cast<const char*>(test_data_parser::itch_bist_pkt2.data())
    );

    auto* msg = std::get<BIST::ITCHAddOrderMessage*>(variant_msg);

    ASSERT_NE(msg, nullptr);
    ASSERT_EQ(msg->message_type, 'A');
    ASSERT_EQ(msg->timestamp_ns, 16);
    ASSERT_EQ(msg->order_id, 1);
    ASSERT_EQ(msg->order_book_id, 2);
    ASSERT_EQ(msg->side, 'B');
    ASSERT_EQ(msg->ranking_seq_number, 1);
    ASSERT_EQ(msg->quantity, 100);
    ASSERT_EQ(msg->price, 50);
    ASSERT_EQ(msg->order_attributes, 1);
    ASSERT_EQ(msg->lot_type, 2);
    ASSERT_EQ(msg->ranking_time, 16);
}

TEST(ITCHBISTParserTest, OrderExecuted)
{
    Parser_ITCH_BIST parser;
    BIST::ITCHMessage variant_msg = parser.parse(
        reinterpret_cast<const char*>(test_data_parser::itch_bist_pkt3.data())
    );

    auto* msg = std::get<BIST::ITCHOrderExecutedMessage*>(variant_msg);

    ASSERT_NE(msg, nullptr);
    ASSERT_EQ(msg->message_type, 'E');
    ASSERT_EQ(msg->timestamp_ns, 32);
    ASSERT_EQ(msg->order_id, 1);
    ASSERT_EQ(msg->order_book_id, 2);
    ASSERT_EQ(msg->side, 'S');
    ASSERT_EQ(msg->executed_quantity, 50);
    ASSERT_EQ(msg->match_id, 0x99);
    ASSERT_EQ(msg->combo_group_id, 3);
}