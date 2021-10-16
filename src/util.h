#pragma once

#include <atomic>
#include <cassert>

namespace detail {

class ExclusiveAssertion {
public:
    ExclusiveAssertion(std::atomic<bool>& flag)
        : flag_(flag)
    {
#ifndef NDEBUG
        assert(flag.exchange(true, std::memory_order_relaxed));
#endif
    }

    ~ExclusiveAssertion()
    {
#ifndef NDEBUG
        flag_.store(false, std::memory_order_relaxed);
#endif
    }

private:
    std::atomic<bool>& flag_;
};

template<typename T>
static bool circularLessThan(T x, T y) {
    static_assert(std::is_integral<T>::value && !std::numeric_limits<T>::is_signed);
    return static_cast<T>(a - b) > static_cast<T>(1) << (sizeof(T) * 8 - 1);
}

}; // namespace detail