#include "Engine/Core/ThreadPool.h"

#include <algorithm>

namespace Engine {

ThreadPool::ThreadPool(std::size_t workerCount) {
    workerCount = std::max<std::size_t>(1, workerCount);
    workers_.reserve(workerCount);
    for (std::size_t index = 0; index < workerCount; ++index) {
        workers_.emplace_back([this]() {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(mutex_);
                    available_.wait(lock, [this]() { return stopping_ || !tasks_.empty(); });
                    if (stopping_ && tasks_.empty()) return;
                    task = std::move(tasks_.front());
                    tasks_.pop();
                }
                task();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
    }
    available_.notify_all();
    for (std::thread& worker : workers_) {
        if (worker.joinable()) worker.join();
    }
}

} // namespace Engine
