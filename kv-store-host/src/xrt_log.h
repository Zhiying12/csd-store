#ifndef XRT_LOG_H_
#define XRT_LOG_H_

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <fcntl.h>
#include <mutex>
#include <optional>
#include <thread>
#include <tuple>
#include <unistd.h>
#include <unordered_map>
#include <vector>
// XRT includes
#include "experimental/xrt_bo.h"
#include "experimental/xrt_device.h"
#include "experimental/xrt_kernel.h"

#include "log.h"
#include "protobuf.h"

const int BUFFER_COUNT = 64;
const int BUFFER_SIZE = 64;
const int STRUCT_FIELDS = 3;
const int VALUE_NUMS = BUFFER_SIZE * STRUCT_FIELDS;
const int KEY_VALUE_SIZE = 8192 * 2;

class XrtLog : public Log {
 public:
  explicit XrtLog(int id,
                  int device_id,
                  cl::Context context,
                  cl::Program program,
                  cl::CommandQueue& queue,
                  std::string store);
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
    cv_appliable_.notify_one();
    apply_thread_.join();
  }

  bool Append(multipaxos::RPC_Instance inst) override;
  void Commit(int64_t index) override;
  std::tuple<int64_t, std::string> Execute() override;

  void EarlyApply();

  // void CommitUntil(int64_t leader_last_executed, int64_t ballot);
  // void TrimUntil(int64_t leader_global_last_executed);

  // std::vector<multipaxos::Instance> Instances() const;

  bool IsExecutable(int64_t i) const {
    return bitmap_[i] == 2;
  }

  // multipaxos::Instance const* at(std::size_t i) const;
  // std::unordered_map<int64_t, multipaxos::Instance> GetLog();

  Instance ConvertInstance(multipaxos::RPC_Instance& i) {
    return Instance(i.ballot(), i.index(), i.client_id(), i.command().type(), 
                    i.command().key(), i.command().value());
  }

 private:
  bool running_ = true;
  cl::Buffer log_bo_;
  // xrt::bo store_bo_;
  int64_t last_index_ = 0;
  int64_t last_executed_ = 0;
  int64_t global_last_executed_ = 0;
  mutable std::mutex mu_;
  std::condition_variable cv_executable_;
  std::condition_variable cv_committable_;
  
  int id_;
  int64_t last_applied_index = 0;
  int64_t current_buffer_index = 0;
  std::vector<int64_t> bitmap_;
  std::vector<std::vector<Instance>> proposals_;
  std::vector<std::unique_ptr<std::atomic<int32_t>>> append_counts_;
  std::vector<std::unique_ptr<std::atomic<int32_t>>> commit_counts_;
  std::condition_variable cv_appliable_;
  std::thread apply_thread_;
  
  // xrt::kernel append_krnl_;
  cl::Kernel execute_krnl_;
  cl::CommandQueue& queue_;
  cl::Buffer result_bo_;
  std::vector<cl::Event> transfer_event_;
  std::vector<cl::Event> execution_event_;

  bool is_persistent_ = false;
  int log_fd_;
  int log_offset_ = 0;
  int store_fd_;
  int store_offset_ = 0;
};

#endif
