#pragma once

#include <cstddef>

template <typename Derived>
class BaseParser
{
public:
    void parser(const char *data, size_t len) noexcept
    {
        static_cast<Derived>(this)->parse(data, len);
    }
};

// class SBEParser : public BaseParser<SBEParser>
// {
// public:
//     inline void parse(const char *data, size_t len) noexcept
//     {
//     }
// };
