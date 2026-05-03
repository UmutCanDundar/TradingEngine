#include "Builder_OUCH_NASDAQ.h"
#include "SoupBinTcp.h"

Builder_OUCH_NASDAQ::Builder_OUCH_NASDAQ(SoupBinTcp &sbt) noexcept : sbt_(sbt) {}

Buffer_ONQ* Builder_OUCH_NASDAQ::build(Order &order) noexcept
{
    constexpr std::array<size_t, 5> msg_sizes = {47, 40, 11, 12, 3};
    Buffer_ONQ *buffer = &messages[next_buffer_slot];

    char *buf = buffer->msg;
    buffer->len = msg_sizes[order.message_type];
    buf = sbt_.WriteSBTHeaderForDataPacket(buf, buffer->len);
    buffer->len += 3;
    (this->*OuchMessageBuilders[order.message_type])(order, buf);

    return buffer;
}