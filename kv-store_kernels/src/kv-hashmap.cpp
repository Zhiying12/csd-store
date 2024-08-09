//#define MAX_SIZE 1024  // Maximum size of the key-value store

struct Entry {
    int index;
    int op;
    int key;
    int value;
};

//KeyValue log[MAX_SIZE];

// Insert a key-value pair into the store
void insert_log(Entry* log, Entry entry, int buffer_size) {
//#pragma HLS PIPELINE
    int index = entry.index;
    log[index] = entry;
}

// Top function for synthesis
void log_top(Entry* log, Entry entry, int buffer_size) {
    insert_log(log, entry, buffer_size);
}

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

void kv_store_top(int* kv_store, Entry* log, int* result_bo, int index, int value_nums) {
//#pragma HLS INTERFACE s_axilite port=op
//#pragma HLS INTERFACE s_axilite port=key
//#pragma HLS INTERFACE s_axilite port=value
//#pragma HLS INTERFACE s_axilite port=result
//#pragma HLS INTERFACE s_axilite port=return

    Entry entry = log[index];
    if (entry.op == 0) {
        kv_insert(entry.key, entry.value, kv_store, value_nums);
        *result_bo = entry.value;
    } else {
        *result_bo = 2000;
    }
}
