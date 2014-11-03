/*
 * ay_wgroup.h
 *
 *  Created on: Nov 3, 2014
 *      Author: polter
 */

#pragma once
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>

/*
    Manages a group of worker threads
    Usage:
        WorkerGroup wg(N); // where N is the number of threads
        wg.run_task(std::function<void()>);
*/

class WorkerGroup {
    typedef std::function<void()> task;
    std::mutex d_mut;
    std::condition_variable d_cv;
    std::queue<task> d_queue;
    unsigned int d_num;
    volatile bool running;
    task get_next();
public:
    WorkerGroup(unsigned int);
    ~WorkerGroup();
    void run_task(task);
};
