#pragma once

#include "Receiver.h"
#include "Parser_Dispatch.h"

class MarketDataHandler
{
private:
    Receiver receiver_;
    Parser_Dispatch parser_;
    // SharedMemoryStore store_;
public:
};
