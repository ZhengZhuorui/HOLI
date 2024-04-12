#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>
class ThreadPool {
public:
    ThreadPool(size_t numThreads) : stop(false), runningThreads(0) {
        for (size_t i = 0; i < numThreads; ++i) {
            threads.emplace_back([this] {
                while (true) {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(queueMutex);
                        condition.wait(lock, [this] {
                            return stop || !tasks.empty();
                        });

                        if (stop && tasks.empty())
                            return;

                        task = std::move(tasks.front());
                        tasks.pop();
                    }

                    // 增加运行中线程计数
                    {
                        std::lock_guard<std::mutex> lock(runningMutex);
                        ++runningThreads;
                    }

                    task();

                    // 减少运行中线程计数
                    {
                        std::lock_guard<std::mutex> lock(runningMutex);
                        --runningThreads;
                    }

                    // 检查是否需要唤醒等待的线程
                    condition.notify_all();
                }
            });
        }
    }

    template <class F, class... Args>
    void enqueue(F&& f, Args&&... args) {
        {
            std::unique_lock<std::mutex> lock(queueMutex);

            if (stop)
                throw std::runtime_error("enqueue on stopped ThreadPool");

            tasks.emplace([f, args...] {
                f(args...);
            });
        }

        condition.notify_one();
    }

    void sync() {
        std::unique_lock<std::mutex> lock(runningMutex);
        condition.wait(lock, [this] {
            return tasks.empty() && runningThreads == 0;
        });
    }

    void stopPool() {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stop = true;
        }

        condition.notify_all();

        for (std::thread& thread : threads)
            thread.join();
    }

    void startPool() {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stop = false;
        }

        for (size_t i = 0; i < threads.size(); ++i) {
            threads[i] = std::thread([this] {
                // 线程工作循环
                // ...
            });
        }
    }

    ~ThreadPool() {
        stopPool();
    }

private:
    std::vector<std::thread> threads;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop;
    std::mutex runningMutex;
    size_t runningThreads;
};