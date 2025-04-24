# CMakeLists.txt
cmake_minimum_required(VERSION 3.10)

project(project4)

set(CMAKE_CXX_STANDARD 17)

include(GenerateProtos.cmake)

add_library(
  worker_clientlib
  worker_client.cc
)
target_link_libraries(worker_clientlib p4protolib)
target_include_directories(worker_clientlib PUBLIC ${MAPREDUCE_INCLUDE_DIR})

add_library(
  mr_taskslib
  mr_tasks.cc
)
target_link_libraries(mr_taskslib p4protolib)
target_include_directories(mr_taskslib PUBLIC ${MAPREDUCE_INCLUDE_DIR})

add_library(
  worker_requestlib
  worker_request.cc
)
target_link_libraries(worker_requestlib p4protolib)
target_include_directories(worker_requestlib PUBLIC ${MAPREDUCE_INCLUDE_DIR})

add_library(
  mapreducelib #library name
  mapreduce.cc mapreduce_impl.cc master.cc #sources
  master.h  mapreduce_spec.h file_shard.h) #headers
target_link_libraries(mapreducelib p4protolib worker_clientlib)
target_include_directories(mapreducelib PUBLIC ${MAPREDUCE_INCLUDE_DIR})
add_dependencies(mapreducelib p4protolib worker_clientlib)

add_library(
  mr_workerlib #library name
  mr_task_factory.cc run_worker.cc worker.cc #sources
  worker.h) #headers
target_link_libraries(mr_workerlib p4protolib mr_taskslib worker_requestlib)
target_include_directories(mr_workerlib PUBLIC ${MAPREDUCE_INCLUDE_DIR})
add_dependencies(mr_workerlib p4protolib mr_taskslib worker_requestlib)
