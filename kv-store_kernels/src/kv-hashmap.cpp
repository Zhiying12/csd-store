#include <stdint.h>
#include <hls_stream.h>
#include "protobuf.h"

#define DATA_SIZE 4096

// TRIPCOUNT identifier
const int c_size = DATA_SIZE;

// void append_instance(Instance* log, Instance* cur_instance_bo, Instance* instance) {
// //#pragma HLS PIPELINE
//   int64_t index = cur_instance_bo->index_;
//   // if (instance->ballot_ > log[index].ballot_) {
//     log[index].ballot_ = cur_instance_bo->ballot_;
//     log[index].client_id_ = cur_instance_bo->client_id_;
//     log[index].command_.type_ = cur_instance_bo->command_.type_;
//     log[index].command_.key_ = cur_instance_bo->command_.key_;
//     log[index].command_.value_ = cur_instance_bo->command_.value_;
//     log[index].index_ = cur_instance_bo->index_;
//     log[index].state_ = cur_instance_bo->state_;
// //    *cur_instance_bo = *instance;
//   // }
// }

// void kv_put(int* kv_store, int key, int value, int value_nums) {
// //#pragma HLS PIPELINE
//     for (int i = 0; i < value_nums; i += 2) {
//         if (kv_store[i] == 0) {
//             kv_store[i] = key;
//             kv_store[i + 1] = value;
//             return;
//         }
//     }
// }

// int kv_get(int* kv_store, int key, int value_nums) {
// //#pragma HLS PIPELINE
//     for (int i = 0; i < value_nums; i += 2) {
//         if (kv_store[i] == key) {
//             return kv_store[i + 1];
//         }
//     }
//     return -1;
// }

// void kv_store_top(int* kv_store, Command* result_bo, Instance* log, int index, int value_nums) {
// //#pragma HLS INTERFACE s_axilite port=op
// //#pragma HLS INTERFACE s_axilite port=key
// //#pragma HLS INTERFACE s_axilite port=value
// //#pragma HLS INTERFACE s_axilite port=result
// //#pragma HLS INTERFACE s_axilite port=return

//     Command cmd = log[index].command_;
//     result_bo->type_ = log[index].client_id_; 
//     result_bo->key_ = cmd.key_;
//     if (cmd.type_ == 0) {
//         kv_put(kv_store, cmd.key_, cmd.value_, value_nums);
//         result_bo->value_ = cmd.value_;
//     } else if (cmd.type_ == 1) {
//         result_bo->value_ = kv_get(kv_store, cmd.key_, value_nums);
//     }
// }

static void read_input(Instance* log, hls::stream<Instance>& inStream, int size) {
// Auto-pipeline is going to apply pipeline to this loop
mem_rd:
    for (int i = 0; i < size; i++) {
#pragma HLS LOOP_TRIPCOUNT min = c_size max = c_size
        inStream << log[i];
    }
}

static void compute_add(hls::stream<Instance>& inStream1,
                        hls::stream<Instance>& outStream,
                        int size) {
// Auto-pipeline is going to apply pipeline to this loop
execute:
    for (int i = 0; i < size; i++) {
#pragma HLS LOOP_TRIPCOUNT min = c_size max = c_size
        outStream << inStream1.read();
    }
}

static void write_result(Instance* result_bo, hls::stream<Instance>& outStream, int size) {
// Auto-pipeline is going to apply pipeline to this loop
mem_wr:
    for (int i = 0; i < size; i++) {
#pragma HLS LOOP_TRIPCOUNT min = c_size max = c_size
        result_bo[i] = outStream.read();
    }
}

void kv_store_find(Instance* log, Instance* result_bo, int size) {
    static hls::stream<Instance> inStream1("input_stream_1");
    static hls::stream<Instance> outStream("output_stream");

#pragma HLS INTERFACE m_axi port = log bundle = gmem0
#pragma HLS INTERFACE m_axi port = result_bo bundle = gmem0

#pragma HLS dataflow
	read_input(log, inStream1, size);
    compute_add(inStream1, outStream, size);
    write_result(result_bo, outStream, size);
}

void kv_store_test(Instance* log, Instance* result_bo, int size) {
	for (int index = 0; index < size; index++) {
	  result_bo[index] = log[index];
	}
}
