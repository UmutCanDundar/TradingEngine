#pragma once

#include <clickhouse/client.h>
#include "Store_RAM.h"

#include <variant>
#include <memory>

class Store_DB
{
public:
    Store_DB(const std::string &host = "127.0.0.1", const int &port = 9000);

    template <typename T>
    void dispatch_insert(const MessageWithVenue<T> &msg) noexcept
    {
        this->insert(msg);
    }

    void dispatch_insert(const MessageWithVenue<std::variant<FIXMessage, ITCHMessage, SBEMessage>> &msgWithVenue) noexcept
    {
        std::visit([this, &msgWithVenue](const auto &inner_msg)
                   {
            using MsgType = std::decay_t<decltype(inner_msg)>;
            this->dispatch_insert(MessageWithVenue<MsgType>{inner_msg, msgWithVenue.venue}); }, msgWithVenue.msg);
    }

    void insert(const Order &order);

private:
    std::unique_ptr<clickhouse::Client> client_;

    inline std::string venue_to_string(Venue venue)
    {
        switch (venue)
        {
        case Venue::NASDAQ:
            return "NASDAQ";
        case Venue::NYSE:
            return "NYSE";
        case Venue::BIST:
            return "BIST";
        default:
            __builtin_unreachable();
        }
    }

    inline std::string protocol_to_string(Protocol protocol)
    {
        switch (protocol)
        {
        case Protocol::FIX:
            return "FIX";
        case Protocol::ITCH:
            return "ITCH";
        case Protocol::SBE:
            return "SBE";
        default:
            __builtin_unreachable();
        }
    }
    inline std::string status_to_string(Status status)
    {
        switch (status)
        {
        case Status::Unknown:
            return "Unknown";
        case Status::New:
            return "New";
        case Status::Partial:
            return "Partial";
        case Status::Filled:
            return "Filled";
        case Status::Cancelled:
            return "Cancelled";

        case Status::PendingNew:
            return "PendingNew";
        case Status::PendingCancel:
            return "PendingCancel";
        case Status::PendingReplace:
            return "PendingReplace";

        case Status::Rejected:
            return "Rejected";
        case Status::CancelReject:
            return "CancelReject";
        case Status::ReplaceReject:
            return "ReplaceReject";

        case Status::Suspended:
            return "Suspended";
        case Status::Expired:
            return "Expired";
        case Status::PartiallyFilled_Cancelled:
            return "PartiallyFilled_Cancelled";

        case Status::DoneForDay:
            return "DoneForDay";
        case Status::Calculated:
            return "Calculated";
        case Status::Stopped:
            return "Stopped";

        case Status::PendingSubmit:
            return "PendingSubmit";
        case Status::Accepted:
            return "Accepted";
        case Status::Restated:
            return "Restated";
        default:
            __builtin_unreachable();
        }
    }

    void insert(const MessageWithVenue<SBEMessage> &sbeMsg);
    void insert(const MessageWithVenue<ITCHMessage> &itchMsg);
    void insert(const MessageWithVenue<FIXMessage> &fixMsg);
};
