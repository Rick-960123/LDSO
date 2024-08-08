
#include "stdio.h"
#include "string"
#include "sstream"
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

    template<class F, class vec>
    void calc(F&& f, vec& data, int size){
        for (int i = 0; i < numThreads; ++i) {
            int start = size/numThreads * i;
            int end = size/numThreads* ( i + 1);
            if (i == 15) end = size;
            enqueue(f, start, end, std::ref(data));
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

void parallelTask(int& start, int& end, std::vector<int>& test_v) {
	for (int i = start; i< end; i++){
		test_v[i] = i;
	}
}

int main(int argc, char* argv[])
{

    int size = 500000;
    printf("vector size %d\n", size);

	std::vector<int> test_v(size);
    std::vector<int> test_v1(size);
    std::vector<int> test_v2(size);
	std::vector<int> test_v3(size);
	std::vector<int> test_v4(size);

	auto m_StartTimepoint = std::chrono::high_resolution_clock::now();

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < size; i++) {    
        test_v[i] = i;
    }

	auto endTimepoint = std::chrono::high_resolution_clock::now();
	auto startNs = std::chrono::time_point_cast<std::chrono::nanoseconds>(m_StartTimepoint).time_since_epoch().count();
	auto endNs = std::chrono::time_point_cast<std::chrono::nanoseconds>(endTimepoint).time_since_epoch().count();

	// 时间差，代码执行时间的纳秒数
	auto ns = endNs - startNs;

	// 微秒
	std::chrono::microseconds micro = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::nanoseconds(ns));
	auto us = micro.count();
	printf("omp:%ld us\n", us);

    auto m_StartTimepoint1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < size; i++) {
    		test_v1[i] = i;
    }

    auto endTimepoint1 = std::chrono::high_resolution_clock::now();
	auto startNs1 = std::chrono::time_point_cast<std::chrono::nanoseconds>(m_StartTimepoint1).time_since_epoch().count();
	auto endNs1 = std::chrono::time_point_cast<std::chrono::nanoseconds>(endTimepoint1).time_since_epoch().count();

	// 时间差，代码执行时间的纳秒数
	auto ns1 = endNs1 - startNs1;

	// 微秒
	std::chrono::microseconds micro1 = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::nanoseconds(ns1));
	auto us1 = micro1.count();
    printf("for:%ld us\n", us1);

    auto m_StartTimepoint2 = std::chrono::high_resolution_clock::now();

	std::for_each(std::execution::par_unseq, test_v2.begin(), test_v2.end(), [startIdx = 0](int& n) mutable {
    	n = startIdx++;
	});

    auto endTimepoint2 = std::chrono::high_resolution_clock::now();
	auto startNs2 = std::chrono::time_point_cast<std::chrono::nanoseconds>(m_StartTimepoint2).time_since_epoch().count();
	auto endNs2 = std::chrono::time_point_cast<std::chrono::nanoseconds>(endTimepoint2).time_since_epoch().count();

	// 时间差，代码执行时间的纳秒数
	auto ns2 = endNs2 - startNs2;

	// 微秒
	std::chrono::microseconds micro2 = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::nanoseconds(ns2));
	auto us2 = micro2.count();
    printf("for_each:%ld us\n", us2);


	ThreadPool pool(16);

	auto m_StartTimepoint3 = std::chrono::high_resolution_clock::now();

	pool.calc(parallelTask, test_v3, test_v3.size());

    auto endTimepoint3 = std::chrono::high_resolution_clock::now();
	auto startNs3 = std::chrono::time_point_cast<std::chrono::nanoseconds>(m_StartTimepoint3).time_since_epoch().count();
	auto endNs3 = std::chrono::time_point_cast<std::chrono::nanoseconds>(endTimepoint3).time_since_epoch().count();

	// 时间差，代码执行时间的纳秒数
	auto ns3 = endNs3 - startNs3;

	// 微秒
	std::chrono::microseconds micro3 = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::nanoseconds(ns3));
	auto us3 = micro3.count();
    printf("threadpool:%ld us\n", us3);

	auto m_StartTimepoint4 = std::chrono::high_resolution_clock::now();

	auto feature = std::async([&](){
		for (int i = 0; i < size; i++) {
			test_v4[i] = i;
		}
	});

	feature.get();

    auto endTimepoint4 = std::chrono::high_resolution_clock::now();
	auto startNs4 = std::chrono::time_point_cast<std::chrono::nanoseconds>(m_StartTimepoint4).time_since_epoch().count();
	auto endNs4 = std::chrono::time_point_cast<std::chrono::nanoseconds>(endTimepoint4).time_since_epoch().count();

	// 时间差，代码执行时间的纳秒数
	auto ns4 = endNs4 - startNs4;

	// 微秒
	std::chrono::microseconds micro4 = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::nanoseconds(ns4));
	auto us4 = micro4.count();
    printf("asynic:%ld us\n", us4);
	return 0;
}