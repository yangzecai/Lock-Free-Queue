#pragma once

#include "util.h"

#include <atomic>
#include <cstdlib>

template<typename T>
class BoundedSpmcQueue {
public:
    BoundedSpmcQueue(int capacity);
    ~BoundedSpmcQueue();

    BoundedSpmcQueue(const BoundedSpmcQueue&) = delete;
    BoundedSpmcQueue& operator=(const BoundedSpmcQueue&) = delete;

    bool tryEnqueue(T&&);
    bool tryEnqueue(const T& t)
    {
        T tmp(t);
        return tryEnqueue(std::move(tmp));
    }
    
    bool tryDequeue(T*);

    size_t size() const;

private:
    bool isCircularLessThanCacheHead(size_t lhs);

    const size_t capacity_;
    T* const elements_;
    std::atomic<bool> enqueueing_;
    std::atomic<size_t> tail_;

    std::atomic<size_t> head_;
    std::atomic<size_t> count_;
};

template<typename T>
BoundedSpmcQueue<T>::BoundedSpmcQueue(int capacity)
    : capacity_(capacity)
    , elements_(std::reinterpret_cast<T*>(std::malloc(sizeof(T)*capacity)))
    , enqueueing_(false)
    , tail_(0)
    , head_(0)
    , count_(0)
{
    // 确保 capacity 为2的N次方
    static_assert(capacity && !(capacity & (capacity - 1)));
}

template<typename T>
BoundedSpmcQueue<T>::~BoundedSpmcQueue()
{
    std::free(elements_);
}

template<typename T>
bool BoundedSpmcQueue<T>::tryEnqueue(T&& t)
{
    // 防止多生产者
    ExclusiveAssertion exclusive(enqueueing_);

    size_t newTail = tail_.load(std::memory_order_relaxed) + 1;
    if (newTail != head_.load(std::memory_order_acquire)) { // sync group 1
        new(elements_ + (newTail - 1) & (capacity_ - 1)) T(std::move(t));
        tail_.store(newTail, std::memory_order_release) // sync group 2
        return true;
    } else {
        return false;
    }
}

template<typename T>
bool BoundedSpmcQueue<T>::tryDequeue(T* t)
{
    size_t tail = tail_.load(std::memory_order_relaxed);
    size_t count = count_.load(std::memory_order_relaxed);
    if (detail::circularLessThan(count, tail)) {
        std::atomic_thread_fence(std::memory_order_acquire); // sync group 3
        count = count_.fetch_add(1, std::memory_order_relaxed);
        tail = tail_.load(std::memory_order_acquire); // sync group 2
        if (detail::circularLessThan(count, tail)) {
            size_t index = head_.fetch_add(1, std::memory_order_acq_rel); // sync group 1
            *t = std::move(elements_[index & (capacity_ - 1)]);
            return true;
        } else {
            count_.fetch_sub(1, std::memory_order_release); // sync group 3
        }
    }
    return false;
}