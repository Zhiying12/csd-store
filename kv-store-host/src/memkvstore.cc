#include <string>

#include "memkvstore.h"

namespace kvstore {

std::string MemKVStore::Get(std::string const& key) {
  auto it = map_.find(key);
  if (it != map_.end())
    return it->second;
  return "";
}

bool MemKVStore::Put(std::string const& key, std::string const& value) {
  map_[key] = value;
  return true;
}

bool MemKVStore::Del(std::string const& key) {
  return map_.erase(key) != 0;
}

}  // namespace kvstore
