#include <grpcpp/grpcpp.h>
#include <mr_task_factory.h>

#include <string>
#include <thread>

#include "masterworker.grpc.pb.h"

using grpc::ServerAsyncResponseWriter;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;

using masterworker::MapQuery;
using masterworker::MapResult;
using masterworker::MasterWorker;
using masterworker::PingQuery;
using masterworker::PingResult;
using masterworker::ReduceQuery;
using masterworker::ReduceResult;

extern std::shared_ptr<BaseMapper> get_mapper_from_task_factory(
    const std::string& user_id);
extern std::shared_ptr<BaseReducer> get_reducer_from_task_factory(
    const std::string& user_id);

class Request {
 public:
  virtual void Proceed() = 0;
  Request(MasterWorker::AsyncService* service, ServerCompletionQueue* cq,
          std::string ip_addr)
      : worker_ip_addr_(ip_addr), service_(service), cq_(cq), status_(CREATE) {}

 protected:
  std::string worker_ip_addr_;
  MasterWorker::AsyncService* service_;
  ServerCompletionQueue* cq_;
  ServerContext ctx_;
  enum RequestStatus { CREATE, PROCESS, FINISH };
  RequestStatus status_;
};

class MapRequest final : public Request {
 public:
  MapRequest(MasterWorker::AsyncService* service, ServerCompletionQueue* cq,
             std::string ip_addr);
  void Proceed();

 private:
  MapResult handle_map_job(const MapQuery&);
  BaseMapperInternal* get_base_mapper_internal(BaseMapper*);

  MapQuery request_;
  MapResult reply_;
  ServerAsyncResponseWriter<MapResult> responder_;
};

class ReduceRequest final : public Request {
 public:
  ReduceRequest(MasterWorker::AsyncService* service, ServerCompletionQueue* cq,
                std::string ip_addr);
  void Proceed();

 private:
  ReduceResult handle_reduce_job(const ReduceQuery&);
  BaseReducerInternal* get_base_reducer_internal(BaseReducer*);

  ReduceQuery request_;
  ReduceResult reply_;
  ServerAsyncResponseWriter<ReduceResult> responder_;
};

class PingRequest final : public Request {
 public:
  PingRequest(MasterWorker::AsyncService* service, ServerCompletionQueue* cq,
              std::string ip_addr);

  void Proceed();

 private:
  PingResult handle_monitor_job(const PingQuery&);

  PingQuery request_;
  PingResult reply_;
  ServerAsyncResponseWriter<PingResult> responder_;
};