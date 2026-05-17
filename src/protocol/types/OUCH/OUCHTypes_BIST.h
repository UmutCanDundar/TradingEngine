#include "GeneratedOUCHMessages_BIST.h"

#include <variant>
#include <tuple>

namespace BIST
{

    using OUCHMessageTypes = std::tuple<
        BIST::RX::OUCHOrderAcceptedMessage,
        BIST::RX::OUCHOrderRejectedMessage,
        BIST::RX::OUCHOrderReplacedMessage,
        BIST::RX::OUCHOrderCancelledMessage,
        BIST::RX::OUCHOrderExecutedMessage>;

    using OUCHMessage = std::variant<
        BIST::RX::OUCHOrderAcceptedMessage *,
        BIST::RX::OUCHOrderRejectedMessage *,
        BIST::RX::OUCHOrderReplacedMessage *,
        BIST::RX::OUCHOrderCancelledMessage *,
        BIST::RX::OUCHOrderExecutedMessage *>;
}