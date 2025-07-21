#include <cassert>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "thread_pool.h"

static void worker(TheadPool* tp) {
    while (true) {
        Work work;
        {
            std::unique_lock<std::mutex> lock(tp->mu);
            tp->not_empty.wait(lock, [&] {
                return tp->stop || !tp->queue.empty();
            });

            if (tp->stop && tp->queue.empty())
                return;

            work = std::move(tp->queue.front());
            tp->queue.pop_front();
        }
        work.task();  // Execute the work
    }
}

void thread_pool_init(TheadPool* tp, size_t num_threads) {
    assert(num_threads > 0);
    tp->stop = false;
    for (size_t i = 0; i < num_threads; ++i) {
        tp->threads.emplace_back(worker, tp);
    }
}

void thread_pool_queue(TheadPool* tp, void (*f)(void*), void* arg) {
    {
        std::lock_guard<std::mutex> lock(tp->mu);
        tp->queue.push_back(Work{ [=]() { f(arg); } });
    }
    tp->not_empty.notify_one();
}

void thread_pool_destroy(TheadPool* tp) {
    {
        std::lock_guard<std::mutex> lock(tp->mu);
        tp->stop = true;
    }
    tp->not_empty.notify_all();

    for (auto& t : tp->threads) {
        if (t.joinable())
            t.join();
    }
}
