#include "master.h"

#include <unistd.h>

#include "client_call.h"
#include "masterworker.grpc.pb.h"

/* CS6210_TASK: This is all the information your master will get from the
   framework. You can populate your other class data members here if you want */
Master::Master(const MapReduceSpec &mr_spec,
               const std::vector<FileShard> &file_shards) {
  num_workers = mr_spec.n_workers;
  shards = std::move(file_shards);
  num_shards = shards.size();
  output_dir = mr_spec.output_dir;
  user_id = mr_spec.user_id;
  num_partitions = mr_spec.n_output_files;
  cq_map_reduce_jobs = new CompletionQueue();
  for (int i = 0; i < mr_spec.n_workers; i++) {
    auto client = std::unique_ptr<WorkerClient>(
        new WorkerClient(mr_spec.worker_ipaddr_ports[i], cq_map_reduce_jobs));
    ip_addr2worker[mr_spec.worker_ipaddr_ports[i]] = std::move(client);
    ready_queue.push(mr_spec.worker_ipaddr_ports[i]);
    worker_ips.push_back(mr_spec.worker_ipaddr_ports[i]);
  }
  map_reduce_flag = false;
  map_complete = false;
  reduce_complete = false;
  trigger_monitoring = false;
  stop_monitoring = false;
  num_dropped_workers = 0;
  num_map_msgs = 0;
  num_reduce_msgs = 0;
  num_dropped_map_jobs = 0;
  num_dropped_reduce_jobs = 0;
}

/* CS6210_TASK: Here you go. once this function is called you will complete
 * whole map reduce task and return true if succeeded */
void Master::monitor() {
  initial_monitor_job = true;
  stop_monitoring = false;
  CompletionQueue cq;
  while (!stop_monitoring) {
    std::unordered_map<std::string, bool> worker_status;
    auto start_time = std::chrono::system_clock::now();

    for (auto ip = worker_ips.begin(); ip != worker_ips.end(); ip++) {
      auto client = ip_addr2worker[*ip].get();
      client->send_monitor_ping(*ip, &cq);
      worker_status[*ip] = false;
    }

    for (int i = 0; i < num_workers; i++) {
      bool error_msg = false;
      std::string worker_ip_received = receive_monitor_pong(&cq, error_msg);
      if (error_msg == false) {
        worker_status[worker_ip_received] = true;
      } else {
        worker_status[worker_ip_received] = false;
        handle_worker_failure(worker_ip_received);
      }
    }

    if (initial_monitor_job) {
      std::unique_lock<std::mutex> lock(m_monitor);
      initial_monitor_job = false;
      cv_monitor.notify_one();
    }

    if (trigger_monitoring) {
      std::unique_lock<std::mutex> lock(m_monitor);
      trigger_monitoring = false;
      cv_monitor.notify_one();
    }

    auto end_time = std::chrono::system_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(
                    end_time - start_time)
                    .count();
    if (diff < 1000) {
      std::unique_lock<std::mutex> lock(m_monitor);
      cv_monitor.wait_for(lock, std::chrono::milliseconds(1001 - diff),
                          [this] { return trigger_monitoring; });
    }
  }

  if (trigger_monitoring) {
    std::unique_lock<std::mutex> lock(m_monitor);
    trigger_monitoring = false;
    cv_monitor.notify_one();
  }
  return;
}

void Master::handle_worker_failure(std::string worker_ip) {
  auto remove_worker_ip = [this](std::string &worker_ip) {
    worker2job.erase(worker_ip);
    ip_addr2worker.erase(worker_ip);
    for (auto it = worker_ips.begin(); it != worker_ips.end(); it++) {
      if (*it == worker_ip) {
        worker_ips.erase(it);
        break;
      }
    }
    dropped_worker_ips.push_back(worker_ip);
  };

  if (!map_complete) {
    std::cout << __func__ << ": Map Worker " << worker_ip << " failed"
              << std::endl;
    {
      std::lock_guard<std::mutex> lock(m);
      auto client = ip_addr2worker[worker_ip].get();
      if (!client) return;

      for (int i = 0; i < client->map_shards.size(); i++) {
        unfinished_shards.push_back(client->map_shards[i]);
      }

      num_dropped_map_jobs += client->completed_map_jobs;

      remove_worker_ip(worker_ip);

      // remove intermediate files created by this worker
      std::string intermediate_file_name_base =
          worker_ip.substr(worker_ip.find_first_of(":") + 1);
      for (auto file = intermediate_files.begin();
           file != intermediate_files.end(); file++) {
        if (file->find(intermediate_file_name_base) != std::string::npos) {
          std::remove(file->c_str());
          intermediate_files.erase(file);
          break;
        }
      }
      num_workers--;
      num_dropped_workers++;
    }
  } else {
    std::cout << __func__ << ": Reduce Worker " << worker_ip << " failed"
              << std::endl;
    {
      std::lock_guard<std::mutex> lock(m);
      auto client = ip_addr2worker[worker_ip].get();
      for (int i = 0; i < client->reduce_partitions.size(); i++) {
        unfinished_partitions.push_back(client->reduce_partitions[i]);
        std::string output_file =
            output_dir + "/" + std::to_string(client->reduce_partitions[i]);
        std::remove(output_file.c_str());
      }
      num_dropped_reduce_jobs += client->completed_reduce_jobs;
      remove_worker_ip(worker_ip);
      num_workers--;
      num_dropped_workers++;
    }
  }
}

std::string Master::receive_monitor_pong(CompletionQueue *cq, bool &error_msg) {
  void *tag;
  bool ok = false;

  std::string ret;
  GPR_ASSERT(cq->Next(&tag, &ok));

  auto call = static_cast<MonitorCall *>(tag);
  if (!call->status.ok()) {
    std::cerr << __func__ << ": failed with " << call->status.error_message()
              << " for worker " << call->worker_ip_addr << std::endl;
    error_msg = true;
    ret = call->worker_ip_addr;
  } else {
    ret = call->result.id();
    error_msg = false;
  }
  delete call;
  return ret;
}

void Master::schedule_monitor() {
  {
    std::unique_lock<std::mutex> lock(m_map_reduce);
    map_reduce_flag = true;
    cv_map_reduce.wait(lock, [this] { return !map_reduce_flag; });
  }

  {
    std::unique_lock<std::mutex> lock(m_monitor);
    trigger_monitoring = true;
    cv_monitor.notify_one();
    cv_monitor.wait(lock, [this] { return !trigger_monitoring; });
  }
  {
    std::unique_lock<std::mutex> lock(m_map_reduce);

    if (!map_complete) {
      if ((unfinished_shards.size() == 0) &&
          (num_map_msgs - num_dropped_map_jobs) == num_shards) {
        map_complete = true;
      }
    } else if (!reduce_complete) {
      if ((unfinished_partitions.size() == 0) &&
          (output_files.size() == num_partitions)) {
        reduce_complete = true;
      }
    }

    map_reduce_flag = true;
    num_dropped_workers = 0;
    cv_map_reduce.notify_one();
  }

  return;
}

void Master::poll_workers() {
  void *tag;
  bool ok = false;
  while (cq_map_reduce_jobs->Next(&tag, &ok)) {
    auto call = static_cast<ClientCall *>(tag);
    if (call->status.ok()) {
      if (std::find(worker_ips.begin(), worker_ips.end(),
                    call->worker_ip_addr) == worker_ips.end()) {
        std::cout << __func__ << ": Discard response from dropped worker "
                  << call->worker_ip_addr << std::endl;
        goto cleanup;
      } else {
        std::cout << __func__ << ": Received response from worker "
                  << call->worker_ip_addr << std::endl;
        {
          std::lock_guard<std::mutex> lock(m);
          ready_queue.push(call->worker_ip_addr);
          worker2job.erase(call->worker_ip_addr);
          cv.notify_one();
        }
        auto client = ip_addr2worker[call->worker_ip_addr].get();
        if (!client) {
          goto cleanup;
        }
        if (call->is_map_job) {
          num_map_msgs++;
          client->completed_map_jobs++;
          auto map_call = dynamic_cast<MapCall *>(call);
          for (int i = 0; i < map_call->result.files_size(); i++) {
            intermediate_files.insert(map_call->result.files(i).filename());
          }
        } else {
          num_reduce_msgs++;
          client->completed_reduce_jobs++;
          auto reduce_call = dynamic_cast<ReduceCall *>(call);
          output_files.push_back(reduce_call->result.file().filename());
        }
      }
    } else {
      std::cerr << __func__
                << ": Received an error: " << call->status.error_message()
                << " from worker " << call->worker_ip_addr << std::endl;
      if (call->is_map_job) {
        num_dropped_map_jobs++;
      } else {
        num_dropped_reduce_jobs++;
      }
    }
  cleanup:
    delete call;
    bool notify_master = false;
    {
      std::unique_lock<std::mutex> lock(m_map_reduce);

      if (((num_dropped_workers == 0) &&
           ((num_map_msgs - num_dropped_map_jobs) == num_shards) &&
           !map_complete) ||
          ((num_dropped_workers == 0) &&
           ((num_reduce_msgs - num_dropped_reduce_jobs) == num_partitions) &&
           !reduce_complete) ||
          (num_dropped_workers > 0)) {
        notify_master = true;
      }

      bool is_map = false;
      if (map_reduce_flag && notify_master) {
        cv_map_reduce.notify_one();
        map_reduce_flag = false;
        if (!map_complete) {
          is_map = true;
        } else {
          is_map = false;
        }

        cv_map_reduce.wait(lock, [this] { return map_reduce_flag; });
        map_reduce_flag = false;
      }

      if ((map_complete && is_map) || (reduce_complete && !is_map)) {
        return;
      }
    }
  }
}

bool Master::run() {
  std::thread monitoring_thread(&Master::monitor, this);
  std::thread spawn_map_jobs(&Master::poll_workers, this);

  int status = mkdir("intermediate", 0777);
  if (status == -1) {
    if (errno == EEXIST) {
      std::cout << "Intermediate directory already exists" << std::endl;
    } else {
      std::cerr << "Error creating intermediate directory" << std::endl;
      return false;
    }
  }

  {
    std::unique_lock<std::mutex> lock(m_monitor);
    initial_monitor_job = true;
    cv_monitor.wait(lock, [this] { return !initial_monitor_job; });
  }

  num_dropped_workers = 0;
  bool complete_all_shards = false;
  while (!complete_all_shards) {
    for (int i = 0; i < shards.size(); i++) {
      FileShard shard = shards[i];
      {
        std::unique_lock<std::mutex> lock(m);
        cv.wait(lock, [this] { return !ready_queue.empty(); });
        std::string worker_ip = ready_queue.front();
        ready_queue.pop();
        // if the worker has been dropped, skip it
        if (std::find(dropped_worker_ips.begin(), dropped_worker_ips.end(),
                      worker_ip) != dropped_worker_ips.end()) {
          i--;
          continue;
        }

        worker2job[worker_ip] = i;
        lock.unlock();
        cv.notify_one();

        auto client = ip_addr2worker[worker_ip].get();
        client->send_mapper_job(user_id, num_partitions, shard);
      }
    }

    schedule_monitor();
    if ((unfinished_shards.size() == 0) && map_complete) {
      shards.clear();
      complete_all_shards = true;
    }

    if (unfinished_shards.size() > 0) {
      shards.clear();
      shards = std::move(unfinished_shards);
    }
  }

  spawn_map_jobs.join();
  std::cout << __func__ << ": Map jobs completed" << std::endl;

  std::unordered_map<int, std::vector<std::string>> idx2files;
  for (auto f : intermediate_files) {
    int start = f.find_first_of('/') + 1;
    int end = f.find_first_of('_');
    int idx = std::stoi(f.substr(start, end - start));
    if (idx2files.find(idx) == idx2files.end()) {
      idx2files[idx] = std::vector<std::string>();
    }
    idx2files[idx].push_back(f);
  }

  for (auto [idx, _] : idx2files) {
    partitions.push_back(idx);
  }

  std::thread spawn_reduce_jobs(&Master::poll_workers, this);

  bool complete_all_partitions = false;
  while (!complete_all_partitions) {
    for (int i = 0; i < partitions.size(); i++) {
      std::unique_lock<std::mutex> lock(m);
      cv.wait(lock, [this] { return !ready_queue.empty(); });
      std::string worker_ip = ready_queue.front();
      ready_queue.pop();

      if (std::find(dropped_worker_ips.begin(), dropped_worker_ips.end(),
                    worker_ip) != dropped_worker_ips.end()) {
        i--;
        continue;
      }

      worker2job[worker_ip] = partitions[i];
      lock.unlock();
      cv.notify_one();

      auto client = ip_addr2worker[worker_ip].get();
      auto files = idx2files[partitions[i]];
      client->send_reducer_job(user_id, partitions[i], output_dir, files);
    }

    schedule_monitor();
    if ((unfinished_partitions.size() == 0) && reduce_complete) {
      partitions.clear();
      complete_all_partitions = true;
    }

    if (unfinished_partitions.size() > 0) {
      partitions.clear();
      partitions = std::move(unfinished_partitions);
    }
  }

  spawn_reduce_jobs.join();
  std::cout << __func__ << ": Reduce jobs completed" << std::endl;

  // stop the monitoring thread
  {
    std::unique_lock<std::mutex> lock(m_monitor);
    stop_monitoring = true;
    trigger_monitoring = true;
    cv_monitor.notify_one();
  }

  monitoring_thread.join();
  std::cout << __func__ << ": Monitoring thread completed" << std::endl;

  std::cout << __func__
            << ": Deleting intermediate files: " << intermediate_files.size()
            << std::endl;
  for (auto f : intermediate_files) {
    std::remove(f.c_str());
  }
  status = rmdir("intermediate");
  if (status == -1) {
    if (errno != ENOTEMPTY) {
      std::cerr << "Error deleting intermediate directory with err: " << errno
                << std::endl;
      return false;
    }
  }
  return true;
}