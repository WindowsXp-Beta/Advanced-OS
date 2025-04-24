#include <grpcpp/grpcpp.h>

#include <memory>
#include <string>

#include "file_shard.h"
#include "masterworker.grpc.pb.h"

using grpc::CompletionQueue;

class WorkerClient {
 public:
  WorkerClient(std::string ip_addr, CompletionQueue* cq) {
    ip_addr_ = ip_addr;
    stub_ = masterworker::MasterWorker::NewStub(
        grpc::CreateChannel(ip_addr, grpc::InsecureChannelCredentials()));
    cq_ = cq;
    completed_map_jobs = 0;
    completed_reduce_jobs = 0;
  }

  void send_mapper_job(std::string, int, FileShard);
  void send_reducer_job(std::string, int, std::string,
                        std::vector<std::string>);
  void send_monitor_ping(std::string, CompletionQueue*);
  std::vector<FileShard> map_shards;
  std::vector<int> reduce_partitions;
  int completed_map_jobs;
  int completed_reduce_jobs;

 private:
  std::unique_ptr<masterworker::MasterWorker::Stub> stub_;
  std::mutex m;
  CompletionQueue* cq_;
  std::string ip_addr_;
};