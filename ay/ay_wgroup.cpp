/*
 * ay_wgroup.cpp
 *
 *  Created on: Nov 3, 2014
 *      Author: polter
 */


#include "ay_wgroup.h"
#include <atomic>

WorkerGroup::WorkerGroup(unsigned int n) : d_num(n), running(true) {
    while (n-- > 0) {
        std::thread([this]() {
            while (running) {
                get_next()();
            }
        }).detach();
    }
}

// to make sure all threads have quit before the group instance is no more
WorkerGroup::~WorkerGroup() {
    std::atomic<int> n(d_num-1);
    {
        std::lock_guard<std::mutex> lk(d_mut);
        d_queue.push([this]() {
            running = false;
        });
        for (unsigned int i = 1; i < d_num; ++i) {
            d_queue.push([&n]() {
                --n;
            });
        }
    }
    d_cv.notify_all();
    while (n > 0)
        ; // waiting until all threads are finished
}

WorkerGroup::task WorkerGroup::get_next() {
    std::unique_lock<std::mutex> lk(d_mut);
    while (d_queue.empty()) {
        d_cv.wait(lk);
    }
    task t = d_queue.front();
    d_queue.pop();
    return t;
}

void WorkerGroup::run_task(task t) {
    {
        std::lock_guard<std::mutex> lk(d_mut);
        d_queue.push(t);
    }
    d_cv.notify_one();
}

