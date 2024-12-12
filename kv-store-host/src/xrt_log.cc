//#include <glog/logging.h>
#include <iostream>
#include "xrt_log.h"

XrtLog::XrtLog(int id,
               int device_id,
               cl::Context context, 
               cl::Program program,
               cl::CommandQueue& queue,
               std::string store)
    : id_(id),
      bitmap_(BUFFER_COUNT, 0),
      proposals_(BUFFER_COUNT, std::vector<Instance>(BUFFER_SIZE)),
      append_counts_(BUFFER_COUNT),
      commit_counts_(BUFFER_COUNT),
      queue_(queue),
      transfer_event_(1),
      execution_event_(1) {
  cl_int err;
  execute_krnl_ = cl::Kernel(program, "kv_store_find", &err);

  log_bo_ = cl::Buffer(context, CL_MEM_READ_ONLY, 
      BUFFER_SIZE * sizeof(Instance), nullptr, &err);
  result_bo_ = cl::Buffer(context, CL_MEM_WRITE_ONLY, 
      BUFFER_SIZE * sizeof(Instance), nullptr, &err);

  err = execute_krnl_.setArg(0, log_bo_);
  err = execute_krnl_.setArg(1, result_bo_);
  err = execute_krnl_.setArg(2, BUFFER_SIZE);

  Instance inst;
  for (int i = 0; i < BUFFER_COUNT; i++) {
    // log_bo_map_[i] = inst;
    // result_bo_map_[i] = inst;
    append_counts_[i] = std::make_unique<std::atomic<int32_t>>(0);
    commit_counts_[i] = std::make_unique<std::atomic<int32_t>>(0);
    for (int j = 0; j < BUFFER_SIZE; j++) {
      proposals_[i][j] = inst;
    }
  }
  apply_thread_ = std::thread(&XrtLog::EarlyApply, this);

  if (store == "file") {
    is_persistent_ = true;
    std::string path = "/export/home/SmartSSD-disk/";
    if (id % 2 == 1)
      path = "/export/home/SmartSSD-disk2/";
    std::string file_name = path + "log" + std::to_string(id);
    log_fd_ = open(file_name.c_str(), O_CREAT | O_RDWR, 0777);
    file_name = path + "store" + std::to_string(id);
    store_fd_ = open(file_name.c_str(), O_CREAT | O_RDWR, 0777);
  }
}

bool XrtLog::Append(multipaxos::RPC_Instance inst) {
  int64_t i = inst.index();
  if (i <= global_last_executed_)
    return true;
  
  auto buffer_index = (i - 1) / BUFFER_SIZE;
  auto buffer_offset = (i - 1) % BUFFER_SIZE;
  buffer_index %= BUFFER_COUNT;

  if (bitmap_[buffer_index] != 0) {
    return false;
  }

  auto instance = ConvertInstance(inst);
  proposals_[buffer_index][buffer_offset] = std::move(instance);
  *append_counts_[buffer_index] += 1;
  if (*append_counts_[buffer_index] == BUFFER_SIZE) {
    std::unique_lock<std::mutex> lock(mu_);
    bitmap_[buffer_index] = 1;
    last_index_ = std::max(last_index_, i);
    cv_appliable_.notify_one();
    cv_committable_.notify_all();
  }

  if (is_persistent_) {
    // auto size = pwrite(log_fd_, (void *)log_bo_map_, 
    //     sizeof(log_bo_map_), log_offset_);
    // log_offset_ += size;
  }
  return true;
}

void XrtLog::Commit(int64_t index) {
  auto buffer_index = ((index - 1) / BUFFER_SIZE) % BUFFER_COUNT;
  *commit_counts_[buffer_index] += 1;
  if (*commit_counts_[buffer_index] == BUFFER_SIZE) {
    std::unique_lock<std::mutex> lock(mu_);
    while (bitmap_[buffer_index] != 1) {
      cv_committable_.wait(lock);
    }
    bitmap_[buffer_index] = 2;
    if (IsExecutable(buffer_index))
      cv_executable_.notify_one();
  }

}

std::tuple<int64_t, std::string> XrtLog::Execute() {
  std::unique_lock<std::mutex> lock(mu_);
  auto buffer_index = (last_executed_ / BUFFER_SIZE) % BUFFER_COUNT;
  while (running_ && !IsExecutable(buffer_index))
    cv_executable_.wait(lock);

  if (!running_)
    return {-1, ""};

  if (last_executed_ >= last_applied_index) {
    std::cout << "slow execution\n";
    cl_int err;
    cl::Event read_event;
    err = queue_.enqueueWriteBuffer(log_bo_, 
                                   CL_FALSE, 
                                   0, 
                                   BUFFER_SIZE * sizeof(Instance), 
                                   proposals_[buffer_index].data(), 
                                   nullptr, &transfer_event_[0]);
    err = queue_.enqueueTask(execute_krnl_, 
                             &transfer_event_, 
                             &execution_event_[0]);
    err = queue_.enqueueReadBuffer(result_bo_, 
                                   CL_FALSE, 
                                   0, 
                                   BUFFER_SIZE * sizeof(Instance), 
                                   proposals_[buffer_index].data(), 
                                   &execution_event_, &read_event);
    if (err != CL_SUCCESS) {
      std::cout << "false execution\n";
    }
    read_event.wait();
    last_applied_index += BUFFER_SIZE;
  }
  if (is_persistent_) {
    // auto size = pwrite(store_fd_, result_bo_map_, 
    //     sizeof(result_bo_map_), store_offset_);
    // store_offset_ += size;
  }
  
  auto offset = last_executed_ % BUFFER_SIZE;
  if (offset == BUFFER_SIZE - 1) {
    *append_counts_[buffer_index] = 0;
    *commit_counts_[buffer_index] = 0;
    bitmap_[buffer_index] = 0;
  }
  ++last_executed_;
  auto kv_result = std::to_string(
      proposals_[buffer_index][offset].command_.value_);
  return {proposals_[buffer_index][offset].client_id_, kv_result};
}

void XrtLog::EarlyApply() {
  cl_int err;
  cl::Event read_event;
  while (true) {
    {
      std::unique_lock<std::mutex> lock(mu_);
      while (running_ && *append_counts_[current_buffer_index] != BUFFER_SIZE) {
        cv_appliable_.wait(lock);
      }
    }
    if (!running_)
      break;
    err = queue_.enqueueWriteBuffer(log_bo_, 
                                   CL_FALSE, 
                                   0, 
                                   BUFFER_SIZE * sizeof(Instance), 
                                   proposals_[current_buffer_index].data(), 
                                   nullptr, &transfer_event_[0]);
    err = queue_.enqueueTask(execute_krnl_, 
                             &transfer_event_, 
                             &execution_event_[0]);
    err = queue_.enqueueReadBuffer(result_bo_, 
                                   CL_FALSE, 
                                   0, 
                                   BUFFER_SIZE * sizeof(Instance), 
                                   proposals_[current_buffer_index].data(), 
                                   &execution_event_, &read_event);
    if (err != CL_SUCCESS) {
      std::cout << "false execution\n";
    }
    read_event.wait();
    {
      std::unique_lock<std::mutex> lock(mu_);
      last_applied_index += BUFFER_SIZE;
      current_buffer_index = (current_buffer_index + 1) % BUFFER_COUNT;
    }
  }
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
