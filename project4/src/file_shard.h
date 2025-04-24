#pragma once

#include <vector>

#include "mapreduce_spec.h"

/* CS6210_TASK: Create your own data structure here, where you can hold
   information about file splits, that your master would use for its own
   bookkeeping and to convey the tasks to the workers for mapping */
struct FileShard {
  std::vector<std::string> files;
  std::vector<std::pair<long, long>>
      offsets;  // (start, end) offsets for each file
};

/* CS6210_TASK: Create fileshards from the list of input files, map_kilobytes
 * etc. using mr_spec you populated  */
inline std::streamsize get_file_size(const std::string& filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);
  if (!file) {
    std::cerr << "Failed to open the file " << filename << std::endl;
    return -1;
  }
  return file.tellg();
}

inline long get_split(const std::string& filename, long offset,
                      int remain_shard_size) {
  auto file_size = get_file_size(filename);
  std::ifstream file(filename);
  if (!file) {
    std::cerr << "Failed to open the file " << filename << std::endl;
    return -1;
  }
  file.seekg(offset + remain_shard_size);
  // fine next '\n'
  std::string line;
  if (!std::getline(file, line)) {
    std::cerr << "Failed to read the file " << filename << std::endl;
    return -1;
  }
  return remain_shard_size + line.size() + 1;  // +1 for '\n'
}

inline bool shard_files(const MapReduceSpec& mr_spec,
                        std::vector<FileShard>& fileShards) {
  auto shard_size = mr_spec.map_kilobytes * 1024;  // in bytes
  FileShard shard;
  auto remain_shard_size = shard_size;
  for (auto f : mr_spec.input_files) {
    auto file_size = get_file_size(f);
    if (file_size == -1) {
      return false;
    }
    long file_offset = 0;
    while (file_size > 0) {
      if (remain_shard_size > file_size) {
        shard.files.push_back(f);
        shard.offsets.push_back({file_offset, file_offset + file_size});
        remain_shard_size -= file_size;
        file_size = 0;
      } else {  // remain_shard_size <= file_size
        auto split_size = get_split(f, file_offset, remain_shard_size);
        if (split_size == -1) {
          return false;
        }
        shard.files.push_back(f);
        shard.offsets.push_back({file_offset, file_offset + split_size});
        file_offset += split_size;
        file_size -= split_size;
        fileShards.push_back(std::move(shard));
        shard = FileShard();
        remain_shard_size = shard_size;
      }
    }
  }
  if (shard.files.size() > 0) {
    fileShards.push_back(std::move(shard));
  }

  for (int j = 0; j < fileShards.size(); j++) {
    auto f = fileShards[j];
    std::cout << "Shard " << j << ": " << std::endl;
    for (int i = 0; i < f.files.size(); i++) {
      std::cout << "File: " << f.files[i] << ", Offset: " << f.offsets[i].first
                << ", " << f.offsets[i].second << '\t';
    }
    std::cout << std::endl;
  }

  return true;
}
