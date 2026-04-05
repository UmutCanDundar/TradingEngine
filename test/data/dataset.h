#pragma once

#include <vector>
#include <cstdint>

#include "NetworkPackets.h"

namespace test_data {
extern const std::vector<uint8_t> single_fix_pkt1;
extern const std::vector<uint8_t> single_fix_pkt2;

extern OutPacket fix_outpacket;

extern OutPacket fix_outpacket_partial_1;
extern OutPacket fix_outpacket_partial_2;
extern OutPacket fix_outpacket_partial_3;

extern OutPacket fix_outpacket_partial_4;
extern OutPacket fix_outpacket_partial_5;
extern OutPacket fix_outpacket_partial_6;

extern const std::vector<uint8_t> ouch_bist_pkt1;
extern const std::vector<uint8_t> ouch_bist_pkt2;
extern const std::vector<uint8_t> ouch_bist_pkt3;

extern const std::vector<uint8_t> ouch_nasdaq_pkt1;
extern const std::vector<uint8_t> ouch_nasdaq_pkt2;
extern const std::vector<uint8_t> ouch_nasdaq_pkt3;

extern const std::vector<uint8_t> itch_bist_pkt1;
extern const std::vector<uint8_t> itch_bist_pkt2;
extern const std::vector<uint8_t> itch_bist_pkt3;

extern const std::vector<uint8_t> itch_nasdaq_pkt1;
extern const std::vector<uint8_t> itch_nasdaq_pkt2;
extern const std::vector<uint8_t> itch_nasdaq_pkt3;

}