#ifndef XRT_LOG_H_
#define XRT_LOG_H_

#include <condition_variable>
#include <cstdint>
#include <fcntl.h>
#include <mutex>
#include <optional>
#include <tuple>
#include <unistd.h>
#include <unordered_map>
#include <vector>
// XRT includes
#include "experimental/xrt_bo.h"
#include "experimental/xrt_device.h"
#include "experimental/xrt_kernel.h"

#include "log.h"
// #include "protobuf.h"

const int BUFFER_SIZE = 10000;
const int STRUCT_FIELDS = 3;
const int VALUE_NUMS = BUFFER_SIZE * STRUCT_FIELDS;
const int MAX_BUFFER_SIZE = VALUE_NUMS * 4;


class XrtLog : public Log {
 public:
  explicit XrtLog(int id, xrt::device& device, xrt::uuid& uuid, std::string store);
  XrtLog(XrtLog const& log) = delete;
  XrtLog& operator=(XrtLog const& log) = delete;
  XrtLog(XrtLog&& log) = delete;
  XrtLog& operator=(XrtLog&& log) = delete;

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

  void Append(Instance instance) override;
  void Commit(int64_t index) override;
  std::tuple<int64_t, int64_t> Execute() override;

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
  xrt::bo log_bo_;
  xrt::bo store_bo_;
  int64_t last_index_ = 0;
  int64_t last_executed_ = 0;
  int64_t global_last_executed_ = 0;
  mutable std::mutex mu_;
  std::condition_variable cv_executable_;
  std::condition_variable cv_committable_;
  int id_;
  std::vector<int64_t> bitmap_;
  
  xrt::kernel append_krnl_;
  // xrt::kernel commit_krnl_;
  xrt::kernel execute_krnl_;
  xrt::bo current_instance_bo_;
  Instance* current_instance_bo_map_;
  xrt::bo result_bo_;
  Command* result_bo_map_;

  bool is_persistent_ = false;
  int log_fd_;
  int log_offset_ = 0;
  int store_fd_;
  int store_offset_ = 0;
};

#endif