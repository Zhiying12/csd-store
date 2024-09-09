#include "protobuf.h"

// Insert a key-value pair into the store
void append_instance(Instance* log, Instance* cur_instance_bo, Instance instance) {
//#pragma HLS PIPELINE
  int64_t index = cur_instance_bo->index_;
  // if (instance->ballot_ > log[index].ballot_) {
    log[index] = *cur_instance_bo;
    *cur_instance_bo = instance;
  // }
}

// void kv_put(int key, int value, int* kv_store, int value_nums) {
// //#pragma HLS PIPELINE
//     for (int i = 0; i < value_nums; i += 3) {
//         if (kv_store[i + 2] == 0) {
//             kv_store[i] = key;
//             kv_store[i + 1] = value;
//             kv_store[i + 2] = 1;
//             return;
//         }
//     }
// }

// int kv_get(int key, int* kv_store, int value_nums) {
// //#pragma HLS PIPELINE
//     for (int i = 0; i < value_nums; i += 3) {
//         if (kv_store[i] == key) {
//             return kv_store[i + 1];
//         }
//     }
//     return -1;
// }

// void kv_store_top(int* kv_store, Instance* log, Command* result_bo, int index, int value_nums) {
// //#pragma HLS INTERFACE s_axilite port=op
// //#pragma HLS INTERFACE s_axilite port=key
// //#pragma HLS INTERFACE s_axilite port=value
// //#pragma HLS INTERFACE s_axilite port=result
// //#pragma HLS INTERFACE s_axilite port=return

//     Command cmd = log[index].command_;
//     if (cmd.type_ == 0) {
//         // kv_put(cmd.key_, cmd.value_, kv_store, value_nums);
//         // result_bo->type_ = log[index].client_id_; 
//         // result_bo->key_ = cmd.value_;
//         // result_bo->value_ = cmd.value_;
//     } else if (cmd.type_ == 1) {
//         // result_bo->value_ = kv_get(cmd.key_, kv_store, value_nums);
//     }
// }
