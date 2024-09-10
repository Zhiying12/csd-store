//#include <glog/logging.h>
#include <iostream>
#include "xrt_log.h"

XrtLog::XrtLog(int id, xrt::device& device, xrt::uuid& uuid, std::string store)
    : id_(id),
      bitmap_(BUFFER_SIZE, 0) {
  append_krnl_ = xrt::kernel(device, uuid, "append_instance");
  execute_krnl_ = xrt::kernel(device, uuid, "kv_store_top");

  xrt::bo::flags flags = xrt::bo::flags::device_only;
  store_bo_ = xrt::bo(device, KEY_VALUE_SIZE * sizeof(int), flags, execute_krnl_.group_id(0));
  result_bo_ = xrt::bo(device, sizeof(Command), execute_krnl_.group_id(1));
  result_bo_map_ = result_bo_.map<Command *>();
  log_bo_ = xrt::bo(device, BUFFER_SIZE * sizeof(Instance), execute_krnl_.group_id(2));
  log_bo_map_ = log_bo_.map<Instance *>();
  // current_instance_bo_ = xrt::bo(device, sizeof(Instance), append_krnl_.group_id(1));
  // current_instance_bo_map_ = current_instance_bo_.map<Instance *>();

  if (store == "file") {
    is_persistent_ = true;
    log_fd_ = open("log", O_CREAT | O_RDWR);
    store_fd_ = open("store", O_CREAT | O_RDWR);
  }
}

void XrtLog::Append(multipaxos::RPC_Instance inst) {
  auto instance = ConvertInstance(inst);
  std::unique_lock<std::mutex> lock(mu_);

  int64_t i = instance.index_;
  if (i <= global_last_executed_)
    return;

  if (bitmap_[i] == 0 || bitmap_[i] == 1) {
    // kernel call
    // auto run = append_krnl_(log_bo_, current_instance_bo_, &instance);
    log_bo_map_[i] = instance;
    log_bo_.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    if (is_persistent_) {
      auto size = pwrite(log_fd_, &instance, 
          sizeof(Instance), log_offset_);
      log_offset_ += size;
    }
    bitmap_[i] = 1;
    last_index_ = std::max(last_index_, i);
    cv_committable_.notify_all();
  }
}

void XrtLog::Commit(int64_t index) {
  std::unique_lock<std::mutex> lock(mu_);
  while (bitmap_[index] == 0) {
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

  auto run = execute_krnl_(store_bo_, result_bo_, log_bo_, 
                           last_executed_ + 1, KEY_VALUE_SIZE);
   if (run) {
      run.wait();
      result_bo_.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
      if (is_persistent_) {
        auto size = pwrite(store_fd_, result_bo_map_, 
            sizeof(result_bo_), store_offset_);
        store_offset_ += size;
      }
   } else
     std::cout << "false run in execute\n";
  std::cout << result_bo_map_->key_ << " " << result_bo_map_->value_ << "\n";
  
  ++last_executed_;
  bitmap_[last_executed_] = 3;
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
