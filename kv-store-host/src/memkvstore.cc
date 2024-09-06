#include <string>

#include "memkvstore.h"

namespace kvstore {

int64_t MemKVStore::Get(int64_t const& key) {
  auto it = map_.find(key);
  if (it != map_.end())
    return it->second;
  return -1;
}

bool MemKVStore::Put(int64_t const& key, int64_t const& value) {
  map_[key] = value;
  return true;
}

bool MemKVStore::Del(int64_t const& key) {
  return map_.erase(key) != 0;
}

}  // namespace kvstore
