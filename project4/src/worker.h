#pragma once

#include <grpcpp/grpcpp.h>
#include <mr_task_factory.h>

#include "masterworker.grpc.pb.h"
#include "mr_tasks.h"

using grpc::Server;
using grpc::ServerCompletionQueue;

/* CS6210_TASK: Handle all the task a Worker is supposed to do.
        This is a big task for this project, will test your understanding of map
   reduce */
class Worker {
 public:
  /* DON'T change the function signature of this constructor */
  Worker(std::string ip_addr_port);
  ~Worker();
  /* DON'T change this function's signature */
  bool run();

  static BaseMapperInternal* get_base_mapper_internal(BaseMapper* mapper) {
    return mapper->impl_;
  }

  static BaseReducerInternal* get_base_reducer_internal(BaseReducer* reducer) {
    return reducer->impl_;
  }

 private:
  /* NOW you can add below, data members and member functions as per the need of
   * your implementation*/
  bool process_map_reduce_request();
  void process_ping_request();

  std::string ip_addr_port_;
  std::unique_ptr<ServerCompletionQueue> map_reduce_queue;
  std::unique_ptr<ServerCompletionQueue> ping_queue;
  masterworker::MasterWorker::AsyncService service_;
  std::unique_ptr<Server> server_;
};

extern std::shared_ptr<BaseMapper> get_mapper_from_task_factory(
    const std::string& user_id);
extern std::shared_ptr<BaseReducer> get_reducer_from_task_factory(
    const std::string& user_id);