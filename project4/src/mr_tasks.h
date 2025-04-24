#pragma once

#include <iostream>
#include <string>
#include <vector>

/* CS6210_TASK Implement this data structureas per your implementation.
                You will need this when your worker is running the map task*/
struct BaseMapperInternal {
  /* DON'T change this function's signature */
  BaseMapperInternal();

  /* DON'T change this function's signature */
  void emit(const std::string& key, const std::string& val);

  /* NOW you can add below, data members and member functions as per the need of
   * your implementation*/
  void dump();
  int num_partitions;
  std::vector<std::string> intermediate_files;
  std::vector<std::pair<std::string, std::string>> key_value_pairs;
};

/*-----------------------------------------------------------------------------------------------*/

/* CS6210_TASK Implement this data structureas per your implementation.
                You will need this when your worker is running the reduce task*/
struct BaseReducerInternal {
  /* DON'T change this function's signature */
  BaseReducerInternal();

  /* DON'T change this function's signature */
  void emit(const std::string& key, const std::string& val);

  /* NOW you can add below, data members and member functions as per the need of
   * your implementation*/
  std::string filename;
};
