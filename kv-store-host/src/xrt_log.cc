//#include <glog/logging.h>
#include <iostream>
#include "xrt_log.h"

XrtLog::XrtLog(int id, xrt::device& device, xrt::uuid& uuid, std::string store)
    : id_(id),
      bitmap_(BUFFER_SIZE, 0),
      count_(0) {
  append_krnl_ = xrt::kernel(device, uuid, "append_instance");
  // execute_krnl_ = xrt::kernel(device, uuid, "kv_store_find");

  xrt::bo::flags flags = xrt::bo::flags::p2p;
  // store_bo_ = xrt::bo(device, KEY_VALUE_SIZE * sizeof(int), flags, execute_krnl_.group_id(0));
  // result_bo_ = xrt::bo(device, sizeof(Command), execute_krnl_.group_id(1));
  // result_bo_map_ = result_bo_.map<Command *>();

  log_bo_ = xrt::bo(device, BUFFER_SIZE * sizeof(Instance), flags, append_krnl_.group_id(0));
  log_bo_map_ = log_bo_.map<Instance *>();
  // current_instance_bo_ = xrt::bo(device, sizeof(Instance), append_krnl_.group_id(1));
  // current_instance_bo_map_ = current_instance_bo_.map<Instance *>();

  if (store == "file") {
    is_persistent_ = true;
    std::string file_name = "log" + std::to_string(id);
    log_fd_ = open(file_name.c_str(), O_CREAT | O_RDWR, 0777);
    file_name = "store" + std::to_string(id);
    store_fd_ = open(file_name.c_str(), O_CREAT | O_RDWR, 0777);
  }
}

void XrtLog::Append(multipaxos::RPC_Instance inst) {
  auto instance = ConvertInstance(inst);
  // std::unique_lock<std::mutex> lock(mu_);

  int64_t i = instance.index_;
  if (i <= global_last_executed_)
    return;

  i = (i - 1) % BUFFER_SIZE;
  // if (bitmap_[i] == 0 || bitmap_[i] == 1) {
    // kernel call
    // *current_instance_bo_map_ = instance;
    // current_instance_bo_.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    // auto run = append_krnl_(log_bo_, current_instance_bo_, &instance);
    // run.wait();
    log_bo_map_[i] = instance;
    if (is_persistent_) {
      auto size = pwrite(log_fd_, (void *)current_instance_bo_map_, 
          sizeof(Instance), log_offset_);
      log_offset_ += size;
    }
    last_index_ = std::max(last_index_, instance.index_);
    count_++;
    if (count_ == BUFFER_SIZE) {
      count_ = 0;
      std::fill(bitmap_.begin(), bitmap_.end(), 1);
      log_bo_.sync(XCL_BO_SYNC_BO_TO_DEVICE);
      cv_committable_.notify_all();
    }
  // }
}

void XrtLog::Commit(int64_t index) {
  std::unique_lock<std::mutex> lock(mu_);
  index = (index - 1) % BUFFER_SIZE;
  while (bitmap_[index] != 1) {
    cv_committable_.wait(lock);
  }

  bitmap_[index] = 2;

  if (IsExecutable())
    cv_executable_.notify_one();
}

std::tuple<int64_t, std::string> XrtLog::Execute() {
  std::unique_lock<std::mutex> lock(mu_);
  while (running_ && !IsExecutable())
    cv_executable_.wait(lock);

  if (!running_)
    return {-1, ""};

  // auto i = (last_executed_ + 1) % BUFFER_SIZE;
  // auto run = execute_krnl_(log_bo_, result_bo_, i);
  // if (run) {
  //   run.wait();
  //   result_bo_.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
  //   if (is_persistent_) {
  //     auto size = pwrite(store_fd_, result_bo_map_, 
  //         sizeof(result_bo_), store_offset_);
  //     store_offset_ += size;
  //   }
  // } else
  //   std::cout << "false run in execute\n";
  // //std::cout << result_bo_map_->key_ << " " << result_bo_map_->value_ << "\n";
  
  // ++last_executed_;
  // bitmap_[i] = 3;
  auto kv_result = std::to_string(result_bo_map_->value_);
  return {result_bo_map_->type_, kv_result};
}

// void XrtLog::CommitUntil(int64_t leader_last_executed, int64_t ballot) {
//   CHECK(leader_last_executed >= 0) << "invalid leader_last_executed";
//   CHECK(ballot >= 0) << "invalid ballot";

//   std::unique_lock<std::mutex> lock(mu_);
//   for (auto i = last_executed_ + 1; i <= leader_last_executed; ++i) {
//     auto it = log_.find(i);
//     if (it == log_.end())
//       break;
//     CHECK(ballot >= it->second.ballot()) << "CommitUntil case 2";
//     if (it->second.ballot() == ballot)
//       it->second.set_state(COMMITTED);
//   }
//   if (IsExecutable())
//     cv_executable_.notify_one();
// }

// void XrtLog::TrimUntil(int64_t leader_global_last_executed) {
//   std::scoped_lock lock(mu_);
//   while (global_last_executed_ < leader_global_last_executed) {
//     ++global_last_executed_;
//     auto it = log_.find(global_last_executed_);
//     CHECK(it != log_.end() && IsExecuted(it->second)) << "TrimUntil case 1";
//     log_.erase(it);
//   }
// }

// std::vector<multipaxos::Instance> XrtLog::Instances() const {
//   std::scoped_lock lock(mu_);
//   std::vector<multipaxos::Instance> instances;
//   for (auto i = global_last_executed_ + 1; i <= last_index_; ++i)
//     if (auto it = log_.find(i); it != log_.end())
//       instances.push_back(it->second);
//   return instances;
// }

// Instance const* XrtLog::at(std::size_t i) const {
//   auto it = log_.find(i);
//   return it == log_.end() ? nullptr : &it->second;
// }

// std::unordered_map<int64_t, multipaxos::Instance> XrtLog::GetLog() {
//   std::unordered_map<int64_t, multipaxos::Instance> local_log(log_);
//   return local_log;
// }
