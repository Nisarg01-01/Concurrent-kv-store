#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace kvstore {

class ThreadPool {
public:
    explicit ThreadPool(std::size_t threadCount);
    ~ThreadPool();

    void enqueue(std::function<void()> task);
    void shutdown();

    std::size_t size() const;

private:
    void workerLoop();

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool stop_ = false;
};

} // namespace kvstore
