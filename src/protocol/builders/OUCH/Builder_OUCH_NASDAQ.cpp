//FUNC-PTR NO-PRE_FILL

#include "Builder_OUCH_NASDAQ.h"
#include "SoupBinTcp.h"

Builder_OUCH_NASDAQ::Builder_OUCH_NASDAQ(SoupBinTcp &sbt) noexcept : sbt_(sbt) {}

Buffer_ONQ* Builder_OUCH_NASDAQ::build(Order &order) noexcept
{
    constexpr std::array<size_t, 5> msg_sizes = {50, 43, 14, 15, 6};
    Buffer_ONQ *buffer = &messages[next_buffer_slot++ & (DAILY_OUCH_NASDAQ_MSG_COUNT-1)];

    char *buf = buffer->msg;
    buffer->len = msg_sizes[order.message_type];
    buf = sbt_.WriteSBTHeaderForDataPacket(buf, buffer->len);
    (this->*OuchMessageBuilders[order.message_type])(order, buf);

    return buffer;
}


//FUNC-PTR PRE_FILL

// #include "Builder_OUCH_NASDAQ.h"
// #include "SoupBinTcp.h"

// Builder_OUCH_NASDAQ::Builder_OUCH_NASDAQ(SoupBinTcp &sbt) noexcept : sbt_(sbt) 
// {}

// Buffer_ONQ* Builder_OUCH_NASDAQ::build(Order &order) noexcept
// {
//     Buffer_ONQ *buffer = nullptr;

//     switch(order.message_type)
//     {
//         case 0:
//             buffer = &enter_messages[next_enter++ & (DAILY_ENTER - 1)];
//             break;
//         case 1:
//             buffer = &replace_messages[(next_replace++ & (DAILY_REPLACE - 1))];
//             break;
//         case 2:
//             buffer = &cancel_messages[(next_cancel++ & (DAILY_CANCEL - 1))];
//             break;
//         case 3:
//             buffer = &modify_messages[(next_modify++ & (DAILY_MODIFY - 1))];
//             break;
//         case 4:
//             buffer = &query_messages[(next_query++ & (DAILY_ACCQUERY - 1))];
//             break;
//     }

//     char *buf = buffer->msg;
//     buf = sbt_.WriteSBTHeaderForDataPacket(buf, buffer->len);

//     (this->*OuchMessageBuilders[order.message_type])(order, buf);

//     return buffer;
// }
















