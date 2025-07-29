#pragma once

#include "Parser_FIX.h"
#include "Parser_ITCH.h"
#include "Parser_SBE.h"
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

class Parser_Dispatch
{
private:
    Parser_FIX fixparser_;
    Parser_ITCH itchparser_;
    Parser_SBE sbeparser_;
    spscQueue_t &queue_;

public:
    Parser_Dispatch(spscQueue_t &queue);

    MessageWithVenue<std::variant<FIXMessage, ITCHMessage, SBEMessage>> dispatch() noexcept;
};
