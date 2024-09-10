#pragma once
#include <thread>
#include <queue>
#include <functional>
#include <vector>
#include <mutex>
#include <condition_variable>

class ThreadPool {
    public:
        ThreadPool(size_t numThreads);
        ~ThreadPool();
        void enqueueTask(std::function<void()> task);
    private:
        std::vector<std::thread> workers;
        std::queue<std::function<void()>> tasks;

        std::mutex queueMutex;
        std::condition_variable condition;
        bool stop;

        void workerThread();
};

