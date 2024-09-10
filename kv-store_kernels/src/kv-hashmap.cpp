#include "protobuf.h"

// Insert a key-value pair into the store
void append_instance(Instance* log, Instance* cur_instance_bo, Instance* instance) {
//#pragma HLS PIPELINE
  int64_t index = cur_instance_bo->index_;
  // if (instance->ballot_ > log[index].ballot_) {
    log[index].ballot_ = cur_instance_bo->ballot_;
    log[index].client_id_ = cur_instance_bo->client_id_;
    log[index].command_.type_ = cur_instance_bo->command_.type_;
    log[index].command_.key_ = cur_instance_bo->command_.key_;
    log[index].command_.value_ = cur_instance_bo->command_.value_;
    log[index].index_ = cur_instance_bo->index_;
    log[index].state_ = cur_instance_bo->state_;
    *cur_instance_bo = *instance;
  // }
}

void kv_put(int* kv_store, int key, int value, int value_nums) {
//#pragma HLS PIPELINE
    for (int i = 0; i < value_nums; i += 2) {
        if (kv_store[i] == 0) {
            kv_store[i] = key;
            kv_store[i + 1] = value;
            return;
        }
    }
}

int kv_get(int* kv_store, int key, int value_nums) {
//#pragma HLS PIPELINE
    for (int i = 0; i < value_nums; i += 2) {
        if (kv_store[i] == key) {
            return kv_store[i + 1];
        }
    }
    return -1;
}

void kv_store_top(int* kv_store, Command* result_bo, Instance* log, int index, int value_nums) {
//#pragma HLS INTERFACE s_axilite port=op
//#pragma HLS INTERFACE s_axilite port=key
//#pragma HLS INTERFACE s_axilite port=value
//#pragma HLS INTERFACE s_axilite port=result
//#pragma HLS INTERFACE s_axilite port=return

    Command cmd = log[index].command_;
    result_bo->type_ = log[index].client_id_; 
    result_bo->key_ = cmd.key_;
    if (cmd.type_ == 0) {
        kv_put(kv_store, cmd.key_, cmd.value_, value_nums);
        result_bo->value_ = cmd.value_;
    } else if (cmd.type_ == 1) {
        result_bo->value_ = kv_get(kv_store, cmd.key_, value_nums);
    }
}
