#include <grpcpp/grpcpp.h>

#include "masterworker.grpc.pb.h"

using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::Status;

class ClientCall {
 public:
  bool is_map_job;
  int id;
  ClientContext context;
  Status status;
  std::string worker_ip_addr;
  virtual ~ClientCall() {}
};

class MapCall : public ClientCall {
 public:
  masterworker::MapResult result;
  std::unique_ptr<ClientAsyncResponseReader<masterworker::MapResult>> rpc;
};

class ReduceCall : public ClientCall {
 public:
  masterworker::ReduceResult result;
  std::unique_ptr<ClientAsyncResponseReader<masterworker::ReduceResult>> rpc;
};

class MonitorCall : public ClientCall {
 public:
  masterworker::PingResult result;
  std::unique_ptr<ClientAsyncResponseReader<masterworker::PingResult>> rpc;
};