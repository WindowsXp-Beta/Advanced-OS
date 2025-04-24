#pragma once

#include <grpcpp/grpcpp.h>

#include <chrono>
#include <condition_variable>
#include <queue>
#include <set>
#include <thread>
#include <unordered_map>

#include "file_shard.h"
#include "mapreduce_spec.h"
#include "worker_client.h"

using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;

/* CS6210_TASK: Handle all the bookkeeping that Master is supposed to do.
        This is probably the biggest task for this project, will test your
   understanding of map reduce */
class Master {
 public:
  /* DON'T change the function signature of this constructor */
  Master(const MapReduceSpec&, const std::vector<FileShard>&);

  /* DON'T change this function's signature */
  bool run();

 private:
  /* NOW you can add below, data members and member functions as per the need of
   * your implementation*/
  void monitor();
  std::string receive_monitor_pong(CompletionQueue* cq, bool& error_msg);
  void schedule_monitor();
  void handle_worker_failure(std::string worker_ip);

  void poll_workers();

  CompletionQueue* cq_map_reduce_jobs;

  std::string user_id;
  std::string output_dir;

  bool initial_monitor_job;
  bool stop_monitoring;
  bool trigger_monitoring;

  int num_workers;
  int num_dropped_workers;
  int num_dropped_map_jobs;
  int num_dropped_reduce_jobs;
  int num_map_msgs;
  int num_reduce_msgs;
  int num_shards;
  int num_partitions;

  bool map_complete;
  bool reduce_complete;
  bool map_reduce_flag;

  std::vector<std::string> worker_ips;
  std::vector<std::string> dropped_worker_ips;
  std::unordered_map<std::string, std::unique_ptr<WorkerClient>> ip_addr2worker;
  std::queue<std::string> ready_queue;
  std::vector<FileShard> shards;
  std::vector<FileShard> unfinished_shards;
  std::vector<int> partitions;
  std::vector<int> unfinished_partitions;
  std::unordered_map<std::string, int>
      worker2job;  // worker_ip -> shard(map) or intermediate(reduce)
  std::set<std::string> intermediate_files;
  std::vector<std::string> output_files;

  // mutexes
  std::mutex m;
  std::condition_variable cv;
  std::mutex m_monitor;
  std::condition_variable cv_monitor;
  std::mutex m_map_reduce;
  std::condition_variable cv_map_reduce;
};
