
#include "GeneratedOUCHMessages_NASDAQ.h"

#include <variant>
#include <tuple>

namespace NASDAQ
{

    using OUCHMessageTypes = std::tuple<
        NASDAQ::OUT::OUCHSystemEventMessage,
        NASDAQ::OUT::OUCHOrderAcceptedMessage,
        NASDAQ::OUT::OUCHOrderReplacedMessage,
        NASDAQ::OUT::OUCHOrderCancelledMessage,
        NASDAQ::OUT::OUCHOrderExecutedMessage,
        NASDAQ::OUT::OUCHOrderRejectedMessage,
        NASDAQ::OUT::OUCHBrokenTradeMessage,
        NASDAQ::OUT::OUCHOrderModifiedMessage,
        NASDAQ::OUT::OUCHCancelPendingMessage,
        NASDAQ::OUT::OUCHCancelRejectMessage,
        NASDAQ::OUT::OUCHAccountQueryResponse>;

    using OUCHMessage = std::variant<
        NASDAQ::OUT::OUCHSystemEventMessage *,
        NASDAQ::OUT::OUCHOrderAcceptedMessage *,
        NASDAQ::OUT::OUCHOrderReplacedMessage *,
        NASDAQ::OUT::OUCHOrderCancelledMessage *,
        NASDAQ::OUT::OUCHOrderExecutedMessage *,
        NASDAQ::OUT::OUCHOrderRejectedMessage *,
        NASDAQ::OUT::OUCHBrokenTradeMessage *,
        NASDAQ::OUT::OUCHOrderModifiedMessage *,
        NASDAQ::OUT::OUCHCancelPendingMessage *,
        NASDAQ::OUT::OUCHCancelRejectMessage *,
        NASDAQ::OUT::OUCHAccountQueryResponse *>;
}