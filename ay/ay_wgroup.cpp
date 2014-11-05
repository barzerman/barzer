/*
 * ay_wgroup.cpp
 *
 *  Created on: Nov 3, 2014
 *      Author: polter
 */


#include "ay_wgroup.h"

WorkerGroup::WorkerGroup(unsigned int n) : d_num(n), running(true) {
    while (n-- > 0) {
        std::thread([this]() {
            while (running) {
                get_next()();
            }
            --d_num;
        }).detach();
    }
}

// to make sure all threads have quit before the group instance is no more
WorkerGroup::~WorkerGroup() {
    {
        std::lock_guard<std::mutex> lk(d_mut);
        running = false;
        for (int i = 0; i < d_num; ++i) {
            d_queue.push([]() {});
        }
    }
    d_cv.notify_all();
    while (d_num > 0)
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

