#include <map>
#include <string>

#include "kvstore.h"

namespace kvstore {

class MemKVStore : public KVStore {
 public:
  MemKVStore() = default;
  MemKVStore(MemKVStore const& store) = delete;
  MemKVStore& operator=(MemKVStore const& store) = delete;
  MemKVStore(MemKVStore&& store) = delete;
  MemKVStore& operator=(MemKVStore&& store) = delete;

  int64_t Get(int64_t const& key) override;
  bool Put(int64_t const& key, int64_t const& value) override;
  bool Del(int64_t const& key) override;

 private:
  std::map<int64_t, int64_t> map_;
};

}  // namespace kvstore
