//#include <iostream>
//#define MAX_SIZE 1024  // Maximum size of the key-value store

// Key-Value structure
//struct KeyValue {
//    int key;
//    int value;
//    int valid;  // Flag to indicate if the key-value pair is valid
//};
//
//// Key-Value store
//KeyValue kv_store[MAX_SIZE];

// Insert a key-value pair into the store
void kv_insert(int key, int value, int* kv_store, int value_nums) {
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

// Get the value associated with a key
int kv_get(int key, int* kv_store, int value_nums) {
//#pragma HLS PIPELINE
    for (int i = 0; i < value_nums; i += 3) {
        if (kv_store[i + 2] == 1 && kv_store[i] == key) {
            return kv_store[i + 1];
        }
    }
    return -1;  // Return -1 if the key is not found
}

// Delete a key-value pair from the store
void kv_delete(int key, int* kv_store, int value_nums) {
//#pragma HLS PIPELINE
    for (int i = 0; i < value_nums; i += 3) {
        if (kv_store[i + 2] == 1 && kv_store[i] == key) {
            kv_store[i + 2] = 0;
            return;
        }
    }
}

// Top function for synthesis
void kv_store_top(int* kv_store, int* result_bo, int op, int key, int value, int value_nums) {
//#pragma HLS INTERFACE s_axilite port=op
//#pragma HLS INTERFACE s_axilite port=key
//#pragma HLS INTERFACE s_axilite port=value
//#pragma HLS INTERFACE s_axilite port=result
//#pragma HLS INTERFACE s_axilite port=return

    if (op == 0) {
        kv_insert(key, value, kv_store, value_nums);
        *result_bo = 0;
    } else if (op == 1) {
        *result_bo = kv_get(key, kv_store, value_nums);
    } else if (op == 2) {
        kv_delete(key, kv_store, value_nums);
        *result_bo = 0;
    } else {
	    *result_bo = 2000;
    }
}
