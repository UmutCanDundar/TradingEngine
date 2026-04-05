#pragma once

#define LIKELY(x) __builtin_expect(!!(x), 1)

#define UNLIKELY(x) __builtin_expect(!!(x), 0)

#ifndef RELEASE_BUILD
    #define IF_NOT_RELEASE(...)  do { __VA_ARGS__ } while(0)
#else
    #define IF_NOT_RELEASE(...)  ((void)0)
#endif