#pragma once

#include "FIXParser.h"
#include "ITCHParser.h"
#include "SBEParser.h"
#include "Receiver.h"

#include <variant>

template <typename T>
struct MessageWithVenue
{
    T msg;
    Venue venue;

    MessageWithVenue(const T &msg, Venue ven) : msg(msg), venue(ven) {}
    MessageWithVenue(T &&msg, Venue ven) : msg(std::move(msg)), venue(ven) {}
};

class Parser
{
private:
    FIXParser fixparser_;
    ITCHParser itchparser_;
    SBEParser sbeparser_;
    spscQueue_t &queue_;

public:
    Parser(spscQueue_t &queue) : queue_(queue) {}

    MessageWithVenue<std::variant<FIXMessage, ITCHMessage, SBEMessage>> parse()
    {
        Packet pkt;
        queue_.pop(pkt);
        switch (pkt.protocol)
        {
        case Protocol::FIX:
            return MessageWithVenue<std::variant<FIXMessage, ITCHMessage, SBEMessage>>(fixparser_.parse(pkt.data.data()), pkt.venue);
        case Protocol::ITCH:
            return MessageWithVenue<std::variant<FIXMessage, ITCHMessage, SBEMessage>>(itchparser_.parse(pkt.data.data()), pkt.venue);
        case Protocol::SBE:
            return MessageWithVenue<std::variant<FIXMessage, ITCHMessage, SBEMessage>>(sbeparser_.parse(pkt.data.data()), pkt.venue);
        }
    }
};
