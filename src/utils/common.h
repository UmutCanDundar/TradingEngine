#pragma once

#include <type_traits>
#include <exception>

#define LIKELY(x) __builtin_expect(!!(x), 1)

#define UNLIKELY(x) __builtin_expect(!!(x), 0)

namespace common_function
{
    //conversion function between enum class type and its underlying type
    template<typename Enum>
    requires std::is_enum_v<Enum>
    [[nodiscard]] constexpr auto to_underlying(Enum e) noexcept 
    {
        return static_cast<std::underlying_type_t<Enum>>(e);
    }
}

std::exception_ptr eptr{};
