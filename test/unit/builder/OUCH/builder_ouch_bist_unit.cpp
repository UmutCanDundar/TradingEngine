
#include <gtest/gtest.h>

#include "dataset.h"
#include "SessionManager.h"
#include "SoupBinTcp.h"
#include "Builder_OUCH_BIST.h"

#include <array>
#include <memory>
#include <iomanip>

TEST(OUCHBISTBuilderTest, OuchNewOrder)
{
 Order* order = test_data::ouch_new_order;

    auto sess_mngr   = std::make_unique<SessionManager>();
    auto sbt         = std::make_unique<SoupBinTcp>(*sess_mngr);
    auto builder_ouch_bist = std::make_unique<Builder_OUCH_BIST>(*sbt, *sess_mngr);

    Buffer_OBT* buf = builder_ouch_bist->build(*order);

    const char* p = buf->msg;  
    EXPECT_EQ(p[0], 0x00);
    EXPECT_EQ(p[1], 0x73);
    
    EXPECT_EQ(p[2], 'U');
    
    EXPECT_EQ(p[3], 'O');
    
    EXPECT_EQ(std::string_view(p + 4, 14), std::string_view("CLIENT0000001 ", 14));
    
    EXPECT_EQ(p[18], 0x00);
    EXPECT_EQ(p[19], 0x00);
    EXPECT_EQ(p[20], 0x00);
    EXPECT_EQ(p[21], 0x03);
    
    EXPECT_EQ(p[22], 'B');
    
    EXPECT_EQ(std::string_view(p + 23, 8), 
              std::string_view("\x00\x00\x00\x00\x00\x00\x00\x64", 8));
    
    EXPECT_EQ(p[31], 0x00);
    EXPECT_EQ(p[32], 0x00);
    EXPECT_EQ(p[33], 0x27);
    EXPECT_EQ(p[34], 0x10);

    EXPECT_EQ(p[35], static_cast<char>(TimeInForce::DAY));
    
    EXPECT_EQ(p[36], 0x00);
    
    EXPECT_EQ(std::memcmp(p + 37, "DMACCOUNT1234   ", 16), 0);
    
    for (int i = 53; i < 68; ++i)
        EXPECT_EQ(p[i], ' ');
    
    EXPECT_EQ(std::memcmp(p + 68, "EXCHANGEINFO1234                ", 32), 0);
    
    for (int i = 100; i < 108; ++i)
        EXPECT_EQ(p[i], 0x00);
    
    EXPECT_EQ(p[108], 0x01);
    
    EXPECT_EQ(p[109], 0x00);
    
    EXPECT_EQ(p[110], 0x00);
    
    EXPECT_EQ(p[111], 0x00);
    
    for (int i = 112; i < 115; ++i)
        EXPECT_EQ(p[i], ' ');    
 
    for (int i = 115; i < 117; ++i)
        EXPECT_EQ(p[i], 0x00); 


std::array<size_t, 19> endlines{1, 2, 3, 17, 21, 22, 30, 34, 35, 36, 52, 67, 99, 107, 108, 109, 110, 111, 114};

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