#include "common_log.h"
#include "log.h"
#include "kvstore.h"
#include "xrt_log.h"

Log* CreateLog(int id, xrt::device& device, xrt::uuid& uuid, std::string store) {
    return new XrtLog(id, device, uuid, store);
}

Log* CreateLog(int id, std::unique_ptr<kvstore::KVStore> kv_store, std::string store) {
    return new CommonLog(id, std::move(kv_store), store);
}