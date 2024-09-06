#ifndef COMMON_LOG_H_
#define COMMON_LOG_H_

#include <condition_variable>
#include <cstdint>
#include <fcntl.h>
#include <mutex>
#include <optional>
#include <tuple>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#include "log.h"
#include "kvstore.h"

bool Insert(std::unordered_map<int64_t, Instance>* log, Instance instance);

class CommonLog : public Log {
 public:
  explicit CommonLog(std::unique_ptr<kvstore::KVStore> kv_store, 
                     std::string store)
      : kv_store_(std::move(kv_store)) {
    if (store == "file") {
      is_persistent_ = true;
      log_fd_ = open("log", O_CREAT | O_RDWR | O_APPEND);
      store_fd_ = open("log", O_CREAT | O_RDWR | O_APPEND);
    }
  }
  CommonLog(CommonLog const& log) = delete;
  CommonLog& operator=(CommonLog const& log) = delete;
  CommonLog(CommonLog&& log) = delete;
  CommonLog& operator=(CommonLog&& log) = delete;

  int64_t LastExecuted() const {
    std::unique_lock<std::mutex> lock(mu_);
    return last_executed_;
  }

  int64_t GlobalLastExecuted() const {
    std::unique_lock<std::mutex> lock(mu_);
    return global_last_executed_;
  }

  int64_t AdvanceLastIndex() {
    std::unique_lock<std::mutex> lock(mu_);
    return ++last_index_;
  }

  void SetLastIndex(int64_t last_index) {
    std::unique_lock<std::mutex> lock(mu_);
    last_index_ = std::max(last_index_, last_index);
  }

  int64_t LastIndex() const {
    std::unique_lock<std::mutex> lock(mu_);
    return last_index_;
  }

  void Stop() {
    std::unique_lock<std::mutex> lock(mu_);
    running_ = false;
    cv_executable_.notify_one();
  }

  void Append(Instance instance);
  void Commit(int64_t index);
  std::tuple<int64_t, int64_t> Execute();

  // void CommitUntil(int64_t leader_last_executed, int64_t ballot);
  // void TrimUntil(int64_t leader_global_last_executed);

  // std::vector<multipaxos::Instance> Instances() const;

  bool IsExecutable() const {
    return bitmap_[last_executed_ + 1] == 2;
  }

  // multipaxos::Instance const* at(std::size_t i) const;
  // std::unordered_map<int64_t, multipaxos::Instance> GetLog();

 private:
  bool running_ = true;
  std::unique_ptr<kvstore::KVStore> kv_store_;
  std::unordered_map<int64_t, Instance> log_;
  int64_t last_index_ = 0;
  int64_t last_executed_ = 0;
  int64_t global_last_executed_ = 0;
  mutable std::mutex mu_;
  std::condition_variable cv_executable_;
  std::condition_variable cv_committable_;
  std::vector<int64_t> bitmap_;

  bool is_persistent_ = false;
  int log_fd_;
  int log_offset_ = 0;
  int store_fd_;
  int store_offset_ = 0;
};

#endif
