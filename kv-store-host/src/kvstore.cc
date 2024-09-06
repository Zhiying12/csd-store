#include "json.h"
#include "kvstore.h"
#include "memkvstore.h"

namespace kvstore {

std::unique_ptr<KVStore> CreateStore(nlohmann::json const& config) {
  return std::make_unique<MemKVStore>();
}

KVResult Execute(Command const& command, KVStore* store) {
  if (command.type_ == 0) {
    auto r = store->Get(command.key_);
    if (!r)
      return KVResult{false, -1};
    return KVResult{true, r};
  }

  if (command.type_ == 1) {
    if (store->Put(command.key_, command.value_))
      return KVResult{true, -1};
    return KVResult{false, -1};
  }


  if (store->Del(command.key_))
    return KVResult{true, -1};
  return KVResult{false, -1};
}

}  // namespace kvstore
