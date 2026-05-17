
#include "GeneratedOUCHMessages_NASDAQ.h"

#include <variant>
#include <tuple>

namespace NASDAQ
{

    using OUCHMessageTypes = std::tuple<
        NASDAQ::RX::OUCHSystemEventMessage,
        NASDAQ::RX::OUCHOrderAcceptedMessage,
        NASDAQ::RX::OUCHOrderReplacedMessage,
        NASDAQ::RX::OUCHOrderCancelledMessage,
        NASDAQ::RX::OUCHOrderExecutedMessage,
        NASDAQ::RX::OUCHOrderRejectedMessage,
        NASDAQ::RX::OUCHBrokenTradeMessage,
        NASDAQ::RX::OUCHOrderModifiedMessage,
        NASDAQ::RX::OUCHCancelPendingMessage,
        NASDAQ::RX::OUCHCancelRejectMessage,
        NASDAQ::RX::OUCHAccountQueryResponse>;

    using OUCHMessage = std::variant<
        NASDAQ::RX::OUCHSystemEventMessage *,
        NASDAQ::RX::OUCHOrderAcceptedMessage *,
        NASDAQ::RX::OUCHOrderReplacedMessage *,
        NASDAQ::RX::OUCHOrderCancelledMessage *,
        NASDAQ::RX::OUCHOrderExecutedMessage *,
        NASDAQ::RX::OUCHOrderRejectedMessage *,
        NASDAQ::RX::OUCHBrokenTradeMessage *,
        NASDAQ::RX::OUCHOrderModifiedMessage *,
        NASDAQ::RX::OUCHCancelPendingMessage *,
        NASDAQ::RX::OUCHCancelRejectMessage *,
        NASDAQ::RX::OUCHAccountQueryResponse *>;
}