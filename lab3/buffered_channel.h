#ifndef BUFFERED_CHANNEL_H_
#define BUFFERED_CHANNEL_H_

#include <queue>
#include <mutex>
#include <condition_variable>
#include <stdexcept>
#include <utility>

template<class T>
class BufferedChannel {
public:
    explicit BufferedChannel(int size) : buffer_size_(size), closed_(false) {}

    void Send(T value) {
        std::unique_lock<std::mutex> lock(mutex_);

        cv_.wait(lock, [this]() {
            return closed_ || queue_.size() < buffer_size_;
            });

        if (closed_) {
            throw std::runtime_error("Channel is closed");
        }

        queue_.push(std::move(value));
        cv_.notify_all();
    }

    std::pair<T, bool> Recv() {
        std::unique_lock<std::mutex> lock(mutex_);

        cv_.wait(lock, [this]() {
            return closed_ || !queue_.empty();
            });

        if (!queue_.empty()) {
            T value = std::move(queue_.front());
            queue_.pop();
            cv_.notify_all();
            return { std::move(value), true };
        }

        return { T(), false };
    }

    void Close() {
        std::lock_guard<std::mutex> lock(mutex_);
        closed_ = true;
        cv_.notify_all();
    }

private:
    std::queue<T> queue_;
    int buffer_size_;
    bool closed_;
    std::mutex mutex_;
    std::condition_variable cv_;
};

#endif // BUFFERED_CHANNEL_H_