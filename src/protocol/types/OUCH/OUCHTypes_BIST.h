#include "GeneratedOUCHMessages_BIST.h"

#include <variant>
#include <tuple>

namespace BIST
{

    using OUCHMessageTypes = std::tuple<
        BIST::OUT::OUCHOrderAcceptedMessage,
        BIST::OUT::OUCHOrderRejectedMessage,
        BIST::OUT::OUCHOrderReplacedMessage,
        BIST::OUT::OUCHOrderCancelledMessage,
        BIST::OUT::OUCHOrderExecutedMessage>;

    using OUCHMessage = std::variant<
        BIST::OUT::OUCHOrderAcceptedMessage *,
        BIST::OUT::OUCHOrderRejectedMessage *,
        BIST::OUT::OUCHOrderReplacedMessage *,
        BIST::OUT::OUCHOrderCancelledMessage *,
        BIST::OUT::OUCHOrderExecutedMessage *>;
}