#ifndef BLOCKINGQUEUE_H
#define BLOCKINGQUEUE_H

#include <mutex>
#include <condition_variable>
#include <chrono>
#include <deque>
#include <assert.h>

template <typename T>
class BlockingQueue {
public:
    BlockingQueue() : mutex_(), cond_(), queue_() {

    }

    BlockingQueue(const BlockingQueue&) = delete;
    BlockingQueue& operator=(const BlockingQueue&) = delete;

    void put(const T& x) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push_back(x);
        cond_.notify_one();
    }

    void put(T&& x) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push_back(x);
        cond_.notify_one();
    }

    T take() {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] { return !queue_.empty(); });
        assert(!queue_.empty());
        T front(std::move(queue_.front()));
        queue_.pop_front();
        return front;
    }

    bool take(T& x, int timeout_ms) {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this] { return !queue_.empty(); });
        if (!queue_.empty()) {
            x = std::move(queue_.front());
            queue_.pop_front();
            return true;
        } else {
            return false;
        }
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable cond_;
    std::deque<T> queue_;
};

#endif  //BLOCKINGQUEUE_H

