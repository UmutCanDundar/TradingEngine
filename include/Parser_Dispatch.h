#pragma once

#include "Parser_FIX.h"
#include "Parser_ITCH.h"
#include "Parser_SBE.h"
#include "Receiver.h"

#include <variant>
#include <boost/lockfree/spsc_queue.hpp>

template <typename T>
struct MessageWithVenue
{
    T msg;
    Venue venue;

    MessageWithVenue() noexcept = default;
    MessageWithVenue(const T msg, Venue ven) noexcept : msg(msg), venue(ven) {}
};

inline constexpr size_t MESSAGE_QUEUE_CAPACITY = 1024;
using spscMessageQueue_t = boost::lockfree::spsc_queue<MessageWithVenue<std::variant<FIXMessage *, ITCHMessage, SBEMessage>>, boost::lockfree::capacity<MESSAGE_QUEUE_CAPACITY>>;

class Parser_Dispatch
{
private:
    Parser_FIX fixparser_;
    Parser_ITCH itchparser_;
    Parser_SBE sbeparser_;
    spscPacketQueue_t &receiver_to_parser_;
    spscMessageQueue_t &parser_to_store_;

public:
    Parser_Dispatch(spscPacketQueue_t &receiver_to_parser, spscMessageQueue_t &parser_to_store);

    void dispatch() noexcept;
};
