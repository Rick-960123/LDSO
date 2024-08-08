
#pragma once
#ifndef LDSO_INDEX_THREAD_REDUCE_H_
#define LDSO_INDEX_THREAD_REDUCE_H_

#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include "Settings.h"

using namespace std;
using namespace std::placeholders;

namespace ldso {

    namespace internal {

        /**
         * Multi thread tasks
         * use reduce function to multi threads a given task
         * like removing outliers or activating points
         * @tparam Running
         */
        template<typename Running>
        class IndexThreadReduce {

        public:
            EIGEN_MAKE_ALIGNED_OPERATOR_NEW;

            inline IndexThreadReduce() {
                nextIndex = 0;
                for (int i = 0; i < NUM_THREADS; i++) {
                    isDone[i] = false;
                    gotOne[i] = true;  // this is unuseful viariable
                    workerThreads[i] = thread(&IndexThreadReduce::workerLoop, this, i);
                }
            }

            inline ~IndexThreadReduce() {
                {
                    std::unique_lock<std::mutex> lock(exMutex);
                    running = false;
                    todo_signal.notify_all();
                }

                for (int i = 0; i < NUM_THREADS; ++i) {
                    if (workerThreads[i].joinable())
                        workerThreads[i].join();
                }
            }

            inline void
            reduce(function<void(int, int, Running *, int)> callPerIndex, int first, int end, int stepSize = 0) 
            {   
                std::unique_lock<std::mutex> lock(waitMutex);

                memset(&stats, 0, sizeof(Running));
                
                if(first >= end) return;

                for (int i = 0; i < NUM_THREADS; i++) {
                    isDone[i] = false;
                    gotOne[i] = false;
                }

                if(stepSize == 0)
                    this->stepSize = ((end - first) + NUM_THREADS - 1) / NUM_THREADS;
                else 
                    this->stepSize = stepSize;

                this->callPerIndex = callPerIndex;
                minIndex = first;
                maxIndex = end;
                nextIndex = first;
                todo_signal.notify_all();

                done_signal.wait(lock, [this]() { 
                    for (int i = 0; i < NUM_THREADS; i++) {
                        if (!isDone[i]) return false;
                    }
                    return nextIndex >= maxIndex || !running;
                });

                this->callPerIndex = bind(&IndexThreadReduce::callPerIndexDefault, this, _1, _2, _3, _4);
            }

            Running stats;

        private:
            thread workerThreads[NUM_THREADS];
            bool isDone[NUM_THREADS];
            bool gotOne[NUM_THREADS];

            mutex exMutex, waitMutex;
            condition_variable todo_signal;
            condition_variable done_signal;

            int minIndex =0;
            int maxIndex =0;
            int stepSize =1;
            int nextIndex =0;

            bool running =true;

            function<void(int, int, Running *, int)> callPerIndex;

            void workerLoop(int idx) {
                while (running) {
                    int todo_start = 0;
                    int todo_end = 0;

                    {
                        std::lock_guard<std::mutex> lock(exMutex);
                        todo_start = nextIndex;
                        todo_end = todo_start + stepSize;
                        nextIndex = todo_end;
                    }

                    if (todo_start < maxIndex) {
                        gotOne[idx] = true;
                        Running s;
                        std::memset(&s, 0, sizeof(Running));
                        callPerIndex(todo_start, std::min(todo_end, maxIndex), &s, idx);
                        {
                            std::lock_guard<std::mutex> lock(exMutex);
                            stats += s;
                        }
                    }else{
                        if (!gotOne[idx]){
                            Running s;
                            memset(&s, 0, sizeof(Running));
                            callPerIndex(0, 0, &s, idx);
                            {
                                std::lock_guard<std::mutex> lock(exMutex);
                                stats += s;
                            }
                            gotOne[idx] = true;
                        }

                        isDone[idx] = true;
                        done_signal.notify_all();
                        
                        std::unique_lock<std::mutex> lock(waitMutex);
                        todo_signal.wait(lock);
                    }
                }
            }

            void callPerIndexDefault(int i, int j, Running *k, int tid) {
                printf("ERROR: should never be called....\n");
                assert(false);
            }
        };

    }
}

#endif // LDSO_INDEX_THREAD_REDUCE_H_
