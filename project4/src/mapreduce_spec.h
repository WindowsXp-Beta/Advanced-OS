#pragma once

#include <sys/stat.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

/* CS6210_TASK: Create your data structure here for storing spec from the config
 * file */
struct MapReduceSpec {
  int n_workers;                                 // number of workers
  std::vector<std::string> worker_ipaddr_ports;  // IP:PORT of each worker
  std::vector<std::string> input_files;          // input files
  std::string output_dir;                        // output directory
  int n_output_files;                            // number of output files
  int map_kilobytes;                             // map task size in KB
  std::string user_id;                           // user id
};

/* CS6210_TASK: Populate MapReduceSpec data structure with the specification
 * from the config file */
inline bool read_mr_spec_from_config_file(const std::string& config_filename,
                                          MapReduceSpec& mr_spec) {
  std::ifstream file(config_filename);
  if (!file) {
    std::cerr << "Failed to open the file " << config_filename << std::endl;
    return false;
  }

  std::string line;
  while (std::getline(file, line)) {
    int pos = line.find('=');
    std::string key = line.substr(0, pos);
    std::string value = line.substr(pos + 1, std::string::npos);
    if (key == "n_workers") {
      mr_spec.n_workers = std::stoi(value);
    } else if (key == "worker_ipaddr_ports") {
      std::istringstream iss(value);
      std::string ip_port;
      while (std::getline(iss, ip_port, ',')) {
        mr_spec.worker_ipaddr_ports.push_back(ip_port);
      }
    } else if (key == "input_files") {
      std::istringstream iss(value);
      std::string input_file;
      while (std::getline(iss, input_file, ',')) {
        mr_spec.input_files.push_back(input_file);
      }
    } else if (key == "output_dir") {
      mr_spec.output_dir = value;
    } else if (key == "n_output_files") {
      mr_spec.n_output_files = std::stoi(value);
    } else if (key == "map_kilobytes") {
      mr_spec.map_kilobytes = std::stoi(value);
    } else if (key == "user_id") {
      mr_spec.user_id = value;
    }
  }
  return true;
}

/* CS6210_TASK: validate the specification read from the config file */
inline bool file_exists(const std::string& name) {
  std::ifstream f(name);
  return f.good();
}

inline bool is_directory(const std::string& path) {
  struct stat info;
  if (stat(path.c_str(), &info) != 0) {
    // Cannot access path
    return false;
  }
  return S_ISDIR(info.st_mode);
}

inline bool validate_mr_spec(const MapReduceSpec& mr_spec) {
  if (mr_spec.n_workers <= 0) {
    std::cerr << "Invalid number of workers: " << mr_spec.n_workers
              << std::endl;
    return false;
  }
  if (mr_spec.worker_ipaddr_ports.size() != mr_spec.n_workers) {
    std::cerr << "Number of worker IP:PORTs does not match n_workers"
              << std::endl;
    return false;
  }
  if (mr_spec.input_files.empty()) {
    std::cerr << "No input files specified" << std::endl;
    return false;
  }
  for (auto f : mr_spec.input_files) {
    if (!file_exists(f)) {
      std::cerr << "Input file does not exist: " << f << std::endl;
      return false;
    }
  }
  if (mr_spec.output_dir.empty()) {
    std::cerr << "Output directory not specified" << std::endl;
    return false;
  }
  if (!is_directory(mr_spec.output_dir)) {
    std::cerr << "Output directory does not exist: " << mr_spec.output_dir
              << std::endl;
    return false;
  }
  if (mr_spec.n_output_files <= 0) {
    std::cerr << "Invalid number of output files: " << mr_spec.n_output_files
              << std::endl;
    return false;
  }
  if (mr_spec.map_kilobytes <= 0) {
    std::cerr << "Invalid map task size in KB: " << mr_spec.map_kilobytes
              << std::endl;
    return false;
  }
  if (mr_spec.user_id.empty()) {
    std::cerr << "User ID not specified" << std::endl;
    return false;
  }
  return true;
}
