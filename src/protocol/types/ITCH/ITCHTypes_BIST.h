#include "GeneratedITCHMessages_BIST.h"

#include <variant>
#include <tuple>

namespace BIST
{
    using ITCHMessageTypes = std::tuple<
        BIST::ITCHSecondsMessage,
        BIST::ITCHOrderBookDirectoryMessage,
        BIST::ITCHCombinationOrderBookLegMessage,
        BIST::ITCHTickSizeTableEntryMessage,
        BIST::ITCHShortSellStatusMessage,
        BIST::ITCHSystemEventMessage,
        BIST::ITCHOrderBookStateMessage,
        BIST::ITCHAddOrderMessage,
        BIST::ITCHOrderExecutedMessage,
        BIST::ITCHOrderExecutedWithPriceMessage,
        BIST::ITCHOrderDeleteMessage,
        BIST::ITCHOrderBookFlushMessage,
        BIST::ITCHTradeMessage,
        BIST::ITCHEquilibriumPriceUpdateMessage>;

    using ITCHMessage = std::variant<
        BIST::ITCHSecondsMessage *,
        BIST::ITCHOrderBookDirectoryMessage *,
        BIST::ITCHCombinationOrderBookLegMessage *,
        BIST::ITCHTickSizeTableEntryMessage *,
        BIST::ITCHShortSellStatusMessage *,
        BIST::ITCHSystemEventMessage *,
        BIST::ITCHOrderBookStateMessage *,
        BIST::ITCHAddOrderMessage *,
        BIST::ITCHOrderExecutedMessage *,
        BIST::ITCHOrderExecutedWithPriceMessage *,
        BIST::ITCHOrderDeleteMessage *,
        BIST::ITCHOrderBookFlushMessage *,
        BIST::ITCHTradeMessage *,
        BIST::ITCHEquilibriumPriceUpdateMessage *>;
}