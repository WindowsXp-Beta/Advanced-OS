#include "worker_client.h"

#include "client_call.h"
#include "const.h"

void WorkerClient::send_mapper_job(std::string user_id, int n_partitions,
                                   FileShard shard) {
  masterworker::MapQuery query;
  query.set_user_id(user_id);
  query.set_n_partitions(n_partitions);
  for (int i = 0; i < shard.files.size(); i++) {
    auto partition = query.add_partitions();
    partition->set_filename(shard.files[i]);
    partition->set_start(shard.offsets[i].first);
    partition->set_end(shard.offsets[i].second);
  }

  map_shards.push_back(shard);

  unsigned int timeout = WorkerClientTimeout;
  auto deadline =
      std::chrono::system_clock::now() + std::chrono::seconds(timeout);

  auto call = new MapCall;
  {
    std::unique_lock<std::mutex> lock(m);
    call->rpc = stub_->PrepareAsyncmapper(&call->context, query, cq_);
    call->is_map_job = true;
    call->context.set_deadline(deadline);
    call->worker_ip_addr = ip_addr_;
    call->rpc->StartCall();
    call->rpc->Finish(&call->result, &call->status, (void*)call);
  }
}

void WorkerClient::send_reducer_job(std::string user_id, int partition_id,
                                    std::string output_dir,
                                    std::vector<std::string> files) {
  masterworker::ReduceQuery query;
  query.set_user_id(user_id);
  query.set_partition_id(partition_id);
  query.set_output_dir(output_dir);
  for (int i = 0; i < files.size(); i++) {
    auto file = query.add_files();
    file->set_filename(files[i]);
  }

  auto deadline = std::chrono::system_clock::now() +
                  std::chrono::seconds(WorkerClientTimeout);
  auto call = new ReduceCall;
  {
    std::unique_lock<std::mutex> lock(m);
    call->rpc = stub_->PrepareAsyncreducer(&call->context, query, cq_);
    call->is_map_job = false;
    call->context.set_deadline(deadline);
    call->worker_ip_addr = ip_addr_;
    call->rpc->StartCall();
    call->rpc->Finish(&call->result, &call->status, (void*)call);
  }
}

void WorkerClient::send_monitor_ping(std::string worker_port,
                                     CompletionQueue* cq) {
  masterworker::PingQuery query;
  query.set_id(worker_port);

  auto deadline = std::chrono::system_clock::now() +
                  std::chrono::seconds(WorkerClientTimeout);
  auto call = new MonitorCall;
  {
    std::unique_lock<std::mutex> lock(m);
    call->rpc = stub_->PrepareAsyncping(&call->context, query, cq);
    call->worker_ip_addr = worker_port;
    call->context.set_deadline(deadline);
    call->is_map_job = false;
    call->rpc->StartCall();
    call->rpc->Finish(&call->result, &call->status, (void*)call);
  }
}