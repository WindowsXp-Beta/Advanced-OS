#include "worker_request.h"

#include <fstream>
#include <map>
#include <sstream>

#include "worker.h"

using grpc::Status;

MapRequest::MapRequest(MasterWorker::AsyncService* service,
                       ServerCompletionQueue* cq, std::string ip_addr)
    : Request(service, cq, ip_addr), responder_(&ctx_) {
  // Invoke the serving logic right away.
  Proceed();
}

void MapRequest::Proceed() {
  if (status_ == CREATE) {
    // Make this instance progress to the PROCESS state.
    status_ = PROCESS;

    service_->Requestmapper(&ctx_, &request_, &responder_, cq_, cq_, this);

  } else if (status_ == PROCESS) {
    new MapRequest(service_, cq_, worker_ip_addr_);

    // Process mapper job
    reply_ = handle_map_job(request_);

    status_ = FINISH;
    responder_.Finish(reply_, Status::OK, this);
  } else {
    GPR_ASSERT(status_ == FINISH);
    delete this;
  }
}

MapResult MapRequest::handle_map_job(const MapQuery& request) {
  MapResult result;
  auto mapper = get_mapper_from_task_factory(request.user_id());
  auto base_mapper = get_base_mapper_internal(mapper.get());

  base_mapper->num_partitions = request.n_partitions();
  std::vector<std::string> intermediate_files;
  for (int i = 0; i < request.n_partitions(); i++) {
    intermediate_files.push_back("intermediate/" + std::to_string(i) + "_" +
                                 worker_ip_addr_ + ".txt");
  }
  base_mapper->intermediate_files = intermediate_files;

  for (int i = 0; i < request.partitions_size(); i++) {
    auto partition = request.partitions(i);

    std::ifstream file(partition.filename());
    file.seekg(partition.start());
    int partition_size = partition.end() - partition.start();
    std::string buffer(partition_size, '\0');
    file.read(&buffer[0], partition_size);

    std::stringstream ss(buffer);
    std::string line;
    while (std::getline(ss, line)) {
      mapper->map(line);
    }
    file.close();
  }

  base_mapper->dump();
  for (int i = 0; i < request.n_partitions(); i++) {
    auto file = result.add_files();
    file->set_filename(intermediate_files[i]);
  }

  return result;
}

BaseMapperInternal* MapRequest::get_base_mapper_internal(BaseMapper* mapper) {
  return Worker::get_base_mapper_internal(mapper);
}

ReduceRequest::ReduceRequest(MasterWorker::AsyncService* service,
                             ServerCompletionQueue* cq, std::string ip_addr)
    : Request(service, cq, ip_addr), responder_(&ctx_) {
  // Invoke the serving logic right away.
  Proceed();
}

void ReduceRequest::Proceed() {
  if (status_ == CREATE) {
    // Make this instance progress to the PROCESS state.
    status_ = PROCESS;

    service_->Requestreducer(&ctx_, &request_, &responder_, cq_, cq_, this);

  } else if (status_ == PROCESS) {
    new ReduceRequest(service_, cq_, worker_ip_addr_);

    // Process reducer job
    reply_ = handle_reduce_job(request_);

    status_ = FINISH;
    responder_.Finish(reply_, Status::OK, this);
    if (worker_ip_addr_ == "50056") {
      std::cout << __func__ << ": Finished reducer job for "
                << request_.partition_id() << std::endl;
    }
  } else {
    GPR_ASSERT(status_ == FINISH);
    delete this;
  }
}

ReduceResult ReduceRequest::handle_reduce_job(const ReduceQuery& query) {
  ReduceResult result;
  auto reducer = get_reducer_from_task_factory(query.user_id());
  auto base_reducer = get_base_reducer_internal(reducer.get());

  std::string output_file =
      query.output_dir() + "/" + std::to_string(query.partition_id());
  std::vector<std::string> files;
  base_reducer->filename = output_file;

  std::map<std::string, std::vector<std::string>> word2values;
  for (int i = 0; i < query.files_size(); i++) {
    std::ifstream file(query.files(i).filename());
    if (!file.is_open()) {
      std::cerr << __func__
                << ": Error opening file: " << query.files(i).filename()
                << std::endl;
      continue;
    }
    std::string line;
    while (std::getline(file, line)) {
      int pos = line.find_first_of(',');
      auto key = line.substr(0, pos);
      auto value = line.substr(pos + 2);
      if (word2values.find(key) == word2values.end()) {
        word2values[key] = std::vector<std::string>();
      }
      word2values[key].push_back(value);
    }
    file.close();
  }

  for (auto& [k, v] : word2values) {
    reducer->reduce(k, v);
  }

  auto file = result.mutable_file();
  file->set_filename(output_file);
  return result;
}

BaseReducerInternal* ReduceRequest::get_base_reducer_internal(
    BaseReducer* reducer) {
  return Worker::get_base_reducer_internal(reducer);
}

PingRequest::PingRequest(MasterWorker::AsyncService* service,
                         ServerCompletionQueue* cq, std::string ip_addr)
    : Request(service, cq, ip_addr), responder_(&ctx_) {
  // Invoke the serving logic right away.
  Proceed();
}

void PingRequest::Proceed() {
  if (status_ == CREATE) {
    // Make this instance progress to the PROCESS state.
    status_ = PROCESS;

    service_->Requestping(&ctx_, &request_, &responder_, cq_, cq_, this);

  } else if (status_ == PROCESS) {
    new PingRequest(service_, cq_, worker_ip_addr_);

    // Process heartbeat message
    reply_.set_id(worker_ip_addr_);

    status_ = FINISH;
    responder_.Finish(reply_, Status::OK, this);

  } else {
    GPR_ASSERT(status_ == FINISH);
    delete this;
  }
}