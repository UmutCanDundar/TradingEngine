#include "GeneratedITCHMessages_NASDAQ.h"

#include <variant>
#include <tuple>

namespace NASDAQ
{
    using ITCHMessageTypes = std::tuple<
        NASDAQ::ITCHAddOrderMessage,
        NASDAQ::ITCHAddOrderMPIDMessage,
        NASDAQ::ITCHCancelMessage,
        NASDAQ::ITCHExecutedMessage,
        NASDAQ::ITCHExecutedWithPriceMessage,
        NASDAQ::ITCHDeleteMessage,
        NASDAQ::ITCHReplaceMessage,
        NASDAQ::ITCHTradeMessage,
        NASDAQ::ITCHSystemEventMessage,
        NASDAQ::ITCHStockDirectoryMessage,
        NASDAQ::ITCHTradingStateMessage>;

    using ITCHMessage = std::variant<
        NASDAQ::ITCHAddOrderMessage *,
        NASDAQ::ITCHAddOrderMPIDMessage *,
        NASDAQ::ITCHCancelMessage *,
        NASDAQ::ITCHExecutedMessage *,
        NASDAQ::ITCHExecutedWithPriceMessage *,
        NASDAQ::ITCHDeleteMessage *,
        NASDAQ::ITCHReplaceMessage *,
        NASDAQ::ITCHTradeMessage *,
        NASDAQ::ITCHSystemEventMessage *,
        NASDAQ::ITCHStockDirectoryMessage *,
        NASDAQ::ITCHTradingStateMessage *>;
}