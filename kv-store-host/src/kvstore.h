#ifndef KVSTORE_H_
#define KVSTORE_H_

#include <string>

#include "json_fwd.h"
#include "multipaxos.pb.h"
#include "protobuf.h"

namespace kvstore {

static std::string const kNotFound = "key not found";
static std::string const kPutFailed = "put failed";
static std::string const kEmpty = "";

struct KVResult {
  bool ok_ = false;
  int64_t value_;
};

class KVStore {
 public:
  virtual ~KVStore() = default;
  virtual int64_t Get(int64_t const& key) = 0;
  virtual bool Put(int64_t const& key, int64_t const& value) = 0;
  virtual bool Del(int64_t const& key) = 0;
};

std::unique_ptr<KVStore> CreateStore(nlohmann::json const& config);

KVResult Execute(Command const& command, KVStore* store);

}  // namespace kvstore

#endif
