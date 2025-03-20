#pragma once

#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

using namespace std;

class Threadpool {
 private:
  int num_threads;
  vector<thread> thread_pool;

  queue<function<void(void)>> event_queue;
  mutex queue_lock;

  condition_variable cv;

  // Function that every threads runs. will keep
  void ThreadLoop() {
    function<void(void)> job;
    while (true) {
      {
        unique_lock<mutex> lock(queue_lock);

        cv.wait(lock, [this]() { return !event_queue.empty(); });

        job = event_queue.front();
        event_queue.pop();
      }
      cout << "Thread is executing a job\n";
      job();
    }
  }

 public:
  Threadpool(int num_threads) {
#ifdef DEBUG
    cout << "Threadpool constructor start" << endl;
#endif

    // Taper at max threads possible
    num_threads = max((int)thread::hardware_concurrency(), num_threads);

#ifdef DEBUG
    cout << "Threadpool constructor maxthreads "
         << thread::hardware_concurrency() << endl;

    cout << "Threadpool constructor threads " << num_threads << endl;
#endif

    for (int i = 0; i < num_threads; i++)
      thread_pool.push_back(thread([this] { ThreadLoop(); }));
  }

  void add_job(function<void(void)> job) {
    {
#ifdef DEBUG
      cout << "Threadpool addjob start" << endl;
#endif

      unique_lock<mutex> lock(queue_lock);
      event_queue.push(job);
    }
#ifdef DEBUG
    cout << "Added a job to the queue\n";
#endif

    cv.notify_one();
  }
};
