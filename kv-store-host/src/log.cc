#include "common_log.h"
#include "log.h"
#include "kvstore.h"
#include "xrt_log.h"

Log* CreateLog(int id, 
               int device_id,
               cl::Context context, 
               cl::Program program, 
               cl::CommandQueue& queue, 
               std::string store) {
    return new XrtLog(id, device_id, context, program, queue, store);
}

Log* CreateLog(int id, 
               int device_id, 
               std::unique_ptr<kvstore::KVStore> kv_store, 
               std::string store) {
    return new CommonLog(id, device_id, std::move(kv_store), store);
}