#include "common_log.h"
#include "log.h"
#include "kvstore.h"
#include "xrt_log.h"

// XRT includes
#include "experimental/xrt_bo.h"
#include "experimental/xrt_device.h"
#include "experimental/xrt_kernel.h"

Log* CreateLog(int id, xrt::device& device, xrt::uuid& uuid, std::string store) {
    return new XrtLog(id, device, uuid, store);
}

Log* CreateLog(std::unique_ptr<kvstore::KVStore> kv_store, std::string store) {
    return new CommonLog(std::move(kv_store), store);
}