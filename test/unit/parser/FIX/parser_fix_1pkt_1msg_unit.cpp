#include <gtest/gtest.h>

#include "Parser_FIX.h"
#include "dataset_parser.h"


TEST(FixParserTest, NewOrderSingle)
{
    spscFIXInSessionQueue_t parser_to_fixbuilder_in;
    Parser_FIX parser1{parser_to_fixbuilder_in};

    FIXMessage* msg = parser1.parse<FIXMessage>(
        reinterpret_cast<const char*>(test_data_parser::single_fix_pkt1.data()),
        test_data_parser::single_fix_pkt1.size()
    );

    ASSERT_NE(msg, nullptr);

    ASSERT_EQ(msg->msg_type, 'D');
    ASSERT_EQ(msg->seqnum, 1);

    ASSERT_EQ(msg->quantity, 100);
    ASSERT_EQ(msg->price, 10000);
    ASSERT_EQ(msg->side, 1);

    ASSERT_STREQ(msg->symbol, "GARAN");
    ASSERT_STREQ(msg->cl_ord_id, "O1");

    ASSERT_STREQ(msg->order_id, "");
    ASSERT_STREQ(msg->exec_id, "");
    ASSERT_STREQ(msg->orig_cl_ord_id, "");

    ASSERT_EQ(msg->last_qty, 0);
    ASSERT_EQ(msg->last_price, 0);
    ASSERT_EQ(msg->leaves_qty, 0);
    ASSERT_EQ(msg->filled_qty, 0);
    ASSERT_EQ(msg->instrument_id, 0);

    ASSERT_EQ(msg->exec_type, 0);
    ASSERT_EQ(msg->ord_status, 0);
    ASSERT_EQ(msg->ord_type, 0);
    ASSERT_EQ(msg->time_in_force, 0);
    ASSERT_EQ(msg->cxl_rej_response_to, 0);
    ASSERT_EQ(msg->cxl_rej_reason, 255);
}

TEST(FixParserTest, ExecutionReport)
{
    spscFIXInSessionQueue_t parser_to_fixbuilder_in;
    Parser_FIX parser2{parser_to_fixbuilder_in};

    FIXMessage* msg = parser2.parse<FIXMessage>(
        reinterpret_cast<const char*>(test_data_parser::single_fix_pkt2.data()),
        test_data_parser::single_fix_pkt2.size()
    );

    ASSERT_NE(msg, nullptr);

    ASSERT_EQ(msg->msg_type, '8');
    ASSERT_EQ(msg->seqnum, 3);

    ASSERT_EQ(msg->exec_type, 'F');
    ASSERT_EQ(msg->ord_status, '2');

    ASSERT_EQ(msg->last_qty, 100);
    ASSERT_EQ(msg->last_price, 10000);

    ASSERT_STREQ(msg->cl_ord_id, "O1");

    ASSERT_EQ(msg->quantity, 0);
    ASSERT_EQ(msg->leaves_qty, 0);
    ASSERT_EQ(msg->filled_qty, 0);
    ASSERT_EQ(msg->instrument_id, 0);

    ASSERT_STREQ(msg->symbol, "");
    ASSERT_STREQ(msg->order_id, "");
    ASSERT_STREQ(msg->orig_cl_ord_id, "");
}