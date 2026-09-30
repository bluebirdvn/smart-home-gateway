#ifndef DBUS_THREAD_H
#define DBUS_THREAD_H

#include <queue>
#include <mutex>
#include <condition_variable>

/**
 * @brief thread-safe queue for dbus message processing thread
 * 
 * @tparam T type of data struct in queue
 */

template <typename T>
class ThreadSafeQueue {
private:
    std::queue<T> queue;
    std::mutex queue_lock;
    std::condition_variable cond;
    bool is_stop = false;

public:
    void push(T value) {
        std::lock_guard<std::mutex> lock(queue_lock);
        queue.push(std::move(value));
        cond.notify_one();
    }

    bool pop(T& value) {
        std::unique_lock<std::mutex> lock(queue_lock);
        cond.wait(lock, [this]() { return !queue.empty() || is_stop; });
        
        if (is_stop && queue.empty()) return false; 
        
        value = std::move(queue.front());
        queue.pop();
        return true;
    }

    void stop() {
        std::lock_guard<std::mutex> lock(queue_lock);
        is_stop = true;
        cond.notify_all();
    }
};

#endif 