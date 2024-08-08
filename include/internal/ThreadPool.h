#pragma once
#ifndef LDSO_THREAD_POOL_H_
#define LDSO_THREAD_POOL_H_

#include <vector>
#include <omp.h>
#include <chrono>
#include <ostream>
#include <algorithm>
#include <execution>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <queue>
#include <atomic>
#include <future>

using namespace std;
using namespace std::placeholders;

namespace ldso {
    namespace internal {
        class ThreadPool {
            public:
                ThreadPool(size_t numThreads) : stop(false), numTasks(0), numThreads(numThreads) {
                    for (size_t i = 0; i < numThreads; ++i) {
                        threads.emplace_back(
                            [this] {
                                while (true) {
                                    std::function<void()> task;
                                    {
                                        std::unique_lock<std::mutex> lock(this->queueMutex);
                                        this->condition.wait(lock,
                                            [this] { return this->stop || !this->tasks.empty(); });
                                        if (this->stop && this->tasks.empty())
                                            return;
                                        task = std::move(this->tasks.front());
                                        this->tasks.pop();
                                    }
                                    task();
                                    numTasks--;
                                }
                            }
                        );
                    }
                }

                template<class F, class... Args>
                void enqueue(F&& f, Args&&... args) {
                    auto task = std::make_shared<std::packaged_task<void()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
                    {
                        std::unique_lock<std::mutex> lock(queueMutex);
                        if (stop)
                            throw std::runtime_error("enqueue on stopped ThreadPool");
                        tasks.emplace([task]() { (*task)(); });
                    }
                    numTasks++;
                    condition.notify_one();
                }

                template<class F, class... Args>
                void calc(F&& f, int start_pos, int end_pos, Args&&... args){
                    int size = end_pos - start_pos;
                    int step = (size + numThreads)/numThreads;
                    for (int i = 0; i < numThreads; ++i) {
                        int start = start_pos + step * i;
                        int end = std::min(start + step, end_pos);
                        if (start > end_pos) break;
                        enqueue(std::forward<F>(f), start, end, std::forward<Args>(args)...);
                    }

                    while (!isFinished()) {
                        std::this_thread::sleep_for(std::chrono::microseconds(10));
                    }
                }

                bool isFinished() const {
                    return numTasks == 0;
                }

                ~ThreadPool() {
                    {
                        std::unique_lock<std::mutex> lock(queueMutex);
                        stop = true;
                    }
                    condition.notify_all();
                    for (std::thread &worker : threads)
                        worker.join();
                }

            private:
                std::vector<std::thread> threads;
                std::queue<std::function<void()>> tasks;
                std::mutex queueMutex;
                std::condition_variable condition;
                std::atomic<int> numTasks;
                bool stop;
                int numThreads;
        };
    }
}

#endif // LDSO_INDEX_THREAD_REDUCE_H_