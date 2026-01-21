#pragma once

#include "Parser_Dispatch.h"
#include <atomic>
#include <thread>

class ParserFlow
{
public:
    ParserFlow(Parser_Dispatch &parser_dispatch) noexcept;

    void start() noexcept;
    void stop() noexcept;

private:
    void run() noexcept;

private:
    Parser_Dispatch &parser_dispatch_;
    std::atomic<bool> running_{false};
    std::thread parser_thread_;
};