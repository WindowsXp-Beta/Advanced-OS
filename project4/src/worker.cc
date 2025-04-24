#include "worker.h"

#include <thread>

#include "worker_request.h"

using grpc::ServerBuilder;

/* CS6210_TASK: ip_addr_port is the only information you get when started.
        You can populate your other class data members here if you want */
Worker::Worker(std::string ip_addr_port) : ip_addr_port_(ip_addr_port) {}

Worker::~Worker() {
  server_->Shutdown();
  map_reduce_queue->Shutdown();
}

/* CS6210_TASK: Here you go. once this function is called your woker's job is to
   keep looking for new tasks from Master, complete when given one and again
   keep looking for the next one. Note that you have the access to BaseMapper's
   member BaseMapperInternal impl_ and BaseReduer's member BaseReducerInternal
   impl_ directly, so you can manipulate them however you want when running
   map/reduce tasks*/
bool Worker::process_map_reduce_request() {
  std::string port_num =
      ip_addr_port_.substr(ip_addr_port_.find_first_of(":") + 1);
  new MapRequest(&service_, map_reduce_queue.get(), port_num);
  new ReduceRequest(&service_, map_reduce_queue.get(), port_num);

  void *tag;
  bool ok;
  while (true) {
    GPR_ASSERT(map_reduce_queue->Next(&tag, &ok));
    if (!ok) {
      std::cerr << __func__ << ": Error in map_reduce_queue on worker "
                << ip_addr_port_ << std::endl;
    }
    static_cast<Request *>(tag)->Proceed();
  }
}

void Worker::process_ping_request() {
  void *tag;
  bool ok;

  new PingRequest(&service_, ping_queue.get(), ip_addr_port_);

  while (true) {
    GPR_ASSERT(ping_queue->Next(&tag, &ok));
    GPR_ASSERT(ok);
    static_cast<Request *>(tag)->Proceed();
  }
}

bool Worker::run() {
  ServerBuilder builder;

  builder.AddListeningPort(ip_addr_port_, grpc::InsecureServerCredentials());
  builder.RegisterService(&service_);

  map_reduce_queue = builder.AddCompletionQueue();
  ping_queue = builder.AddCompletionQueue();
  server_ = builder.BuildAndStart();

  std::thread pong_thread(&Worker::process_ping_request, this);

  process_map_reduce_request();
  return true;
}