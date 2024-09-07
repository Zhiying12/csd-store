//#include <glog/logging.h>
#include <iostream>
#include "common_log.h"

// bool IsCommitted(multipaxos::Instance const& instance) {
//   return instance.state == 1;
// }
// bool IsExecuted(multipaxos::Instance const& instance) {
//   return instance.state == 2;
// }
// bool IsInProgress(multipaxos::Instance const& instance) {
//   return instance.state == 0;
// }

bool Insert(std::unordered_map<int64_t, Instance>* log, Instance instance) {
  auto i = instance.index_;
  auto it = log->find(i);
  if (it == log->end()) {
    (*log)[i] = std::move(instance);
    return true;
  }
  // if (IsCommitted(it->second) || IsExecuted(it->second)) {
  //   CHECK(it->second.command() == instance.command()) << "Insert case2";
  //   return false;
  // }
  if (instance.ballot_ > it->second.ballot_) {
    (*log)[i] = std::move(instance);
    return false;
  }
  // if (instance.ballot() == it->second.ballot())
  //   CHECK(it->second.command() == instance.command()) << "Insert case3";
  return false;
}

void CommonLog::Append(Instance instance) {
  std::unique_lock<std::mutex> lock(mu_);

  int64_t i = instance.index_;
  if (i <= global_last_executed_)
    return;

  if (Insert(&log_, std::move(instance))) {
    last_index_ = std::max(last_index_, i);
    cv_committable_.notify_all();
    if (is_persistent_) {
      auto size = pwrite(log_fd_, &instance, sizeof(instance), log_offset_);
      log_offset_ += size;
    }
  }
}

void CommonLog::Commit(int64_t index) {
  std::unique_lock<std::mutex> lock(mu_);
  auto it = log_.find(index);
  while (it == log_.end()) {
    cv_committable_.wait(lock);
    it = log_.find(index);
  }

  if (it->second.state_ == 0)
    it->second.state_ = 1;

  if (IsExecutable())
    cv_executable_.notify_one();
}

std::tuple<int64_t, int64_t> CommonLog::Execute() {
  std::unique_lock<std::mutex> lock(mu_);
  while (running_ && !IsExecutable())
    cv_executable_.wait(lock);

  if (!running_)
    return {-1, -1};

  auto it = log_.find(last_executed_ + 1);
  Instance* instance = &it->second;
  kvstore::KVResult result =
      kvstore::Execute(instance->command_, kv_store_.get());
  ++last_executed_;
  it->second.state_ = 2;
  
  if (is_persistent_) {
    auto size = pwrite(store_fd_, &instance, sizeof(instance), store_offset_);
    store_offset_ += size;
  }
  
  return {instance->client_id_, result.value_};
}

// void CommonLog::CommitUntil(int64_t leader_last_executed, int64_t ballot) {
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

// void CommonLog::TrimUntil(int64_t leader_global_last_executed) {
//   std::scoped_lock lock(mu_);
//   while (global_last_executed_ < leader_global_last_executed) {
//     ++global_last_executed_;
//     auto it = log_.find(global_last_executed_);
//     CHECK(it != log_.end() && IsExecuted(it->second)) << "TrimUntil case 1";
//     log_.erase(it);
//   }
// }

// std::vector<multipaxos::Instance> CommonLog::Instances() const {
//   std::scoped_lock lock(mu_);
//   std::vector<multipaxos::Instance> instances;
//   for (auto i = global_last_executed_ + 1; i <= last_index_; ++i)
//     if (auto it = log_.find(i); it != log_.end())
//       instances.push_back(it->second);
//   return instances;
// }

// Instance const* CommonLog::at(std::size_t i) const {
//   auto it = log_.find(i);
//   return it == log_.end() ? nullptr : &it->second;
// }

// std::unordered_map<int64_t, multipaxos::Instance> CommonLog::GetLog() {
//   std::unordered_map<int64_t, multipaxos::Instance> local_log(log_);
//   return local_log;
// }
