//#include <glog/logging.h>
#include <iostream>
#include "log.h"

// bool IsCommitted(multipaxos::Instance const& instance) {
//   return instance.state == 1;
// }
// bool IsExecuted(multipaxos::Instance const& instance) {
//   return instance.state == 2;
// }
// bool IsInProgress(multipaxos::Instance const& instance) {
//   return instance.state == 0;
// }

// //TODO move Insert and associated functions to kernel
// bool Insert(Instance instance) {
//   auto i = instance.index();
//   auto it = log->find(i);
//   if (it == log->end()) {
//     (*log)[i] = std::move(instance);
//     return true;
//   }
//   if (IsCommitted(it->second) || IsExecuted(it->second)) {
//     CHECK(it->second.command() == instance.command()) << "Insert case2";
//     return false;
//   }
//   if (instance.ballot() > it->second.ballot()) {
//     (*log)[i] = std::move(instance);
//     return false;
//   }
//   if (instance.ballot() == it->second.ballot())
//     CHECK(it->second.command() == instance.command()) << "Insert case3";
//   return false;
// }

Log::Log()
    : bitmap_(BUFFER_SIZE, 0) {
  //TODO: init xrt::bo and open file
  auto binaryFile = "kv-store.xclbin";
  std::string dev_id = "1";

  device_ = xrt::device(dev_id);
  auto uuid = device_.load_xclbin(binaryFile);
  append_krnl_ = xrt::kernel(device_, uuid, "append_instance");
  execute_krnl_ = xrt::kernel(device_, uuid, "kv_store_top");

  log_bo_ = xrt::bo(device_, BUFFER_SIZE * sizeof(Instance), append_krnl_.group_id(0));
  store_bo_ = xrt::bo(device_, MAX_BUFFER_SIZE, execute_krnl_.group_id(0));
}

void Log::Append(Instance instance) {
  std::unique_lock<std::mutex> lock(mu_);

  int64_t i = instance.index_;
  if (i <= global_last_executed_)
    return;

  if (bitmap_[i] == 0 || bitmap_[i] == 1) {
    // kernel call
    auto run = append_krnl_(&log_bo_, &instance, BUFFER_SIZE);
    if (run) {
      run.wait();
    } else
      std::cout << "false run in append\n";
    bitmap_[i] = 1;
    last_index_ = std::max(last_index_, i);
    cv_committable_.notify_all();
  }
}

void Log::Commit(int64_t index) {
  std::unique_lock<std::mutex> lock(mu_);
  while (bitmap_[index] == 0) {
    cv_committable_.wait(lock);
  }

  bitmap_[index] = 2;

  if (IsExecutable())
    cv_executable_.notify_one();
}

std::tuple<int64_t, int64_t> Log::Execute() {
  std::unique_lock<std::mutex> lock(mu_);
  while (running_ && !IsExecutable())
    cv_executable_.wait(lock);

  int64_t kv_result = -1;
  if (!running_)
    return {-1, kv_result};

  auto result_bo = xrt::bo(device_, 4, execute_krnl_.group_id(0));
  int* result_bo_map = result_bo.map<int *>();
  auto run = execute_krnl_(&store_bo_, &log_bo_, &result_bo_map, 
                           last_executed_ + 1, MAX_BUFFER_SIZE);
   if (run) {
      run.wait();
      result_bo.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
   } else
     std::cout << "false run in execute\n";
  
  ++last_executed_;
  bitmap_[last_executed_] = 3;
  return {kv_result, kv_result};
}

// void Log::CommitUntil(int64_t leader_last_executed, int64_t ballot) {
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

// void Log::TrimUntil(int64_t leader_global_last_executed) {
//   std::scoped_lock lock(mu_);
//   while (global_last_executed_ < leader_global_last_executed) {
//     ++global_last_executed_;
//     auto it = log_.find(global_last_executed_);
//     CHECK(it != log_.end() && IsExecuted(it->second)) << "TrimUntil case 1";
//     log_.erase(it);
//   }
// }

// std::vector<multipaxos::Instance> Log::Instances() const {
//   std::scoped_lock lock(mu_);
//   std::vector<multipaxos::Instance> instances;
//   for (auto i = global_last_executed_ + 1; i <= last_index_; ++i)
//     if (auto it = log_.find(i); it != log_.end())
//       instances.push_back(it->second);
//   return instances;
// }

// Instance const* Log::at(std::size_t i) const {
//   auto it = log_.find(i);
//   return it == log_.end() ? nullptr : &it->second;
// }

// std::unordered_map<int64_t, multipaxos::Instance> Log::GetLog() {
//   std::unordered_map<int64_t, multipaxos::Instance> local_log(log_);
//   return local_log;
// }
