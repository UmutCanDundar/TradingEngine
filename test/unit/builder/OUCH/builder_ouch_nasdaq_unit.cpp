
#include <gtest/gtest.h>

#include "dataset_builder.h"
#include "SessionManager.h"
#include "SoupBinTcp.h"
#include "Builder_OUCH_NASDAQ.h"

#include <array>
#include <memory>
#include <iomanip>

TEST(OUCHNasdaqBuilderTest, OuchNewOrder)
{
    Order* order = test_data_builder::NQouch_new_order;

    auto sess_mngr   = std::make_unique<SessionManager>();
    auto sbt         = std::make_unique<SoupBinTcp>(*sess_mngr);
    auto builder_ouch_nasdaq = std::make_unique<Builder_OUCH_NASDAQ>(*sbt);

    Buffer_ONQ* buf = builder_ouch_nasdaq->build(*order);

    const char* p = buf->msg;  
    
    EXPECT_EQ(p[0], 0x00);
    EXPECT_EQ(p[1], 0x30);
    
    EXPECT_EQ(p[2], 'U');
    
    EXPECT_EQ(p[3], 'O');

    EXPECT_EQ(p[4], 0x00);
    EXPECT_EQ(p[5], 0x00);
    EXPECT_EQ(p[6], 0x00);
    EXPECT_EQ(p[7], 0x01);

    EXPECT_EQ(p[8], 'S');

    EXPECT_EQ(p[9],  0x00);
    EXPECT_EQ(p[10],  0x00);
    EXPECT_EQ(p[11],  0x00);
    EXPECT_EQ(p[12],  0x64);

    EXPECT_EQ(std::memcmp(p + 13, "AAPL    ", 8), 0);

    EXPECT_EQ(std::string_view(p + 21, 8),
              std::string_view("\x00\x00\x00\x00\x00\x00\x27\x10", 8));

    EXPECT_EQ(p[29], static_cast<char>(TimeInForce::DAY));

    EXPECT_EQ(p[30], 'Y');

    EXPECT_EQ(p[31], 'A');

    EXPECT_EQ(p[32], 'N');

    EXPECT_EQ(p[33], 'N');

    EXPECT_EQ(std::string_view(p + 34, 14), std::string_view("CLIENT0000008 ", 14));

    EXPECT_EQ(p[48], 0x00);
    EXPECT_EQ(p[49], 0x00);


std::array<size_t, 16> endlines{1, 2, 3, 7, 8, 12, 20, 28, 29, 30, 31, 32, 33, 47, 48, 49};

std::cout << std::endl;
std::cout << "sbtHeader[-3]: ";
for(ssize_t i = -3; i < buf->len - 3; i++) 
{
    unsigned char c = static_cast<unsigned char>(buf->msg[i+3]);
    if (c >= 0x20 && c <= 0x7E && i != -3 && i != -2)  // printable ASCII
        std::cout << "'" << static_cast<char>(c) << "'";
    else
        std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') 
                  << static_cast<unsigned int>(c) << std::dec;

    if(std::find(endlines.begin(), endlines.end(), i+3) != endlines.end())
    {
        if((i + 3) != 1)
            std::cout << std::endl << "ouchMsg[" << i+1 << "]: ";
        else
            std::cout << std::endl << "sbtHeader[" << i+1 << "]: ";
    }
    else
        std::cout << " - ";
}
std::cout << std::endl << "buffer_len: " << buf->len << std::endl << std::endl;

}