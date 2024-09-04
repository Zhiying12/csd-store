#include "protobuf.h"

// Insert a key-value pair into the store
void append_instance(Instance* log, Instance* instance, int buffer_size) {
//#pragma HLS PIPELINE
  int64_t index = instance->index_;
  if (instance->ballot_ > log[index].ballot_)
    log[index] = *instance;
}

void kv_put(int key, int value, int* kv_store, int value_nums) {
//#pragma HLS PIPELINE
    for (int i = 0; i < value_nums; i += 3) {
        if (kv_store[i + 2] == 0) {
            kv_store[i] = key;
            kv_store[i + 1] = value;
            kv_store[i + 2] = 1;
            return;
        }
    }
}

int kv_get(int key, int* kv_store, int value_nums) {
//#pragma HLS PIPELINE
    for (int i = 0; i < value_nums; i += 3) {
        if (kv_store[i] == key) {
            return kv_store[i + 1];
        }
    }
    return -1;
}

void kv_store_top(int* kv_store, Instance* log, int* result_bo, int index, int value_nums) {
//#pragma HLS INTERFACE s_axilite port=op
//#pragma HLS INTERFACE s_axilite port=key
//#pragma HLS INTERFACE s_axilite port=value
//#pragma HLS INTERFACE s_axilite port=result
//#pragma HLS INTERFACE s_axilite port=return

    Command cmd = log[index].command_;
    if (cmd.type_ == 0) {
        kv_put(cmd.key_, cmd.value_, kv_store, value_nums);
        *result_bo = cmd.value_;
    } else if (cmd.type_ == 1) {
        *result_bo = kv_get(cmd.key_, kv_store, value_nums);
    }
}
