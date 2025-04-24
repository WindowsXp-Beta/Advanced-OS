#include "mr_tasks.h"

#include <fstream>

#include "const.h"

/* CS6210_TASK Implement this function */
BaseMapperInternal::BaseMapperInternal() {}

/* CS6210_TASK Implement this function */
void BaseMapperInternal::emit(const std::string& key, const std::string& val) {
  if (key_value_pairs.size() >= KeyValuePairDumpThreshold) {
    dump();
  }
  key_value_pairs.push_back({key, val});
}

void BaseMapperInternal::dump() {
  for (auto& [k, v] : key_value_pairs) {
    auto hash = std::hash<std::string>{}(k) % num_partitions;
    auto filename = intermediate_files[hash];
    std::ofstream intermediate_file(filename, std::ios::app | std::ios::out);
    intermediate_file << k << ", " << v << std::endl;
    intermediate_file.close();
  }
  key_value_pairs.clear();
}

/* CS6210_TASK Implement this function */
BaseReducerInternal::BaseReducerInternal() {}

/* CS6210_TASK Implement this function */
void BaseReducerInternal::emit(const std::string& key, const std::string& val) {
  std::ofstream output_file(filename, std::ios::app | std::ios::out);
  output_file << key << " " << val << std::endl;
  output_file.close();
}