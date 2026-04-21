#pragma once

#include <atomic>
#include <cstdint>

inline std::uint64_t allocateObjectId()
{
    static std::atomic<std::uint64_t> s_next{1};
    return ++s_next;
}
