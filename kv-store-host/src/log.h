#ifndef LOG_H
#define LOG_H

#include <cstdint>
#include <unistd.h>

#include "kvstore.h"

// XRT includes
#include "experimental/xrt_bo.h"
#include "experimental/xrt_device.h"
#include "experimental/xrt_kernel.h"

class Log {
 public:
  virtual ~Log() = default;
  virtual int64_t LastExecuted() const = 0;
  virtual int64_t GlobalLastExecuted() const = 0;
  virtual int64_t AdvanceLastIndex() = 0;
  virtual void SetLastIndex(int64_t last_index) = 0;
  virtual int64_t LastIndex() const = 0;
  virtual void Stop() = 0;
  virtual void Append(multipaxos::RPC_Instance instance) = 0;
  virtual void Commit(int64_t index) = 0;
  virtual std::tuple<int64_t, std::string> Execute() = 0;
  virtual bool IsExecutable() const = 0;
};

Log* CreateLog(int id, xrt::device& device, xrt::uuid& uuid, std::string store);
Log* CreateLog(int id, std::unique_ptr<kvstore::KVStore> kv_store, std::string store);

#endif
