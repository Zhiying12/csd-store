//#define MAX_SIZE 1024  // Maximum size of the key-value store

struct Entry {
    int index;
    int op;
    int key;
    int value;
};

//KeyValue kv_store[MAX_SIZE];

// Insert a key-value pair into the store
void kv_insert(Entry* kv_store, Entry entry, int buffer_size) {
//#pragma HLS PIPELINE
    int index = entry.index;
    kv_store[index] = entry;
}

// Get the value associated with a key
int kv_get(Entry* kv_store, Entry entry, int buffer_size) {
//#pragma HLS PIPELINE
    int index = entry.index;
    kv_store[index] = entry;
    return kv_store[index].value + 800;
}

// Top function for synthesis
void kv_store_top(Entry* kv_store, int* result_bo, Entry entry, int buffer_size) {
//#pragma HLS INTERFACE s_axilite port=op
//#pragma HLS INTERFACE s_axilite port=key
//#pragma HLS INTERFACE s_axilite port=value
//#pragma HLS INTERFACE s_axilite port=result
//#pragma HLS INTERFACE s_axilite port=return

    int op = entry.op;
    if (op == 0) {
        kv_insert(kv_store, entry, buffer_size);
        *result_bo = 0;
    } else if (op == 1) {
        *result_bo = kv_get(kv_store, entry, buffer_size);
    } else {
	    *result_bo = 2000;
    }
}
