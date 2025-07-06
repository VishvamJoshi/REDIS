#pragma once

#include <thread>
#include <vector>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <functional>

struct Work
{
    std::function<void()> task; // Encapsulated function with no arguments
};

struct TheadPool
{
    std::vector<std::thread> threads;
    std::deque<Work> queue;
    std::mutex mu;
    std::condition_variable not_empty;
    bool stop = false;
};

void thread_pool_init(TheadPool *tp, size_t num_threads);
void thread_pool_queue(TheadPool *tp, void (*f)(void *), void *arg);
void thread_pool_destroy(TheadPool *tp); // Optional cleanup
