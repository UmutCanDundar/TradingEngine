#pragma once

#include "FIXParser.h"
#include "ITCHParser.h"
#include "SBEParser.h"
#include "Receiver.h"

#include <variant>

class Parser
{
private:
    FIXParser fixparser_;
    ITCHParser itchparser_;
    SBEParser sbeparser_;
    spscQueue_t &queue_;

public:
    Parser(spscQueue_t &queue) : queue_(queue) {}

    std::variant<FIXMessage, ITCHMessage, SBEMessage> parse()
    {
        PacketWithProtocol pkt;
        queue_.pop(pkt);
        switch (pkt.protocol)
        {
        case Protocol::FIX:
            return fixparser_.parse(pkt.data.data());
        case Protocol::ITCH:
            return itchparser_.parse(pkt.data.data());
        case Protocol::SBE:
            return sbeparser_.parse(pkt.data.data());
        }
    }
};
