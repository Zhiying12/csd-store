//#include "cmdlineparser.h"
//#include "xcl2.hpp"
#include <atomic>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <iosfwd>
#include <iostream>
#include <unistd.h>
#include <vector>
#include <chrono>
#include <iostream>
#include <errno.h>
#include <thread>
#include <vector>
// XRT includes
#include "experimental/xrt_bo.h"
#include "experimental/xrt_device.h"
#include "experimental/xrt_kernel.h"

int BUFFER_SIZE = 1024;
int STRUCT_FIELDS = 3;
int VALUE_NUMS = BUFFER_SIZE * STRUCT_FIELDS;
int MAX_BUFFER_SIZE = VALUE_NUMS * 4;

struct Entry {
    int index;
    int op;
    int key;
    int value;
};

void kv_store_apply(xrt::bo& kernel_bo, xrt::bo& result_bo, xrt::kernel& krnl, Entry& entry) {
	auto run = krnl(kernel_bo, result_bo, entry, BUFFER_SIZE);
	if (run) {
	  run.wait();
	  result_bo.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
	} else
	  std::cout << "false run\n";
}

int main(int argc, char** argv) {
    // Command Line Parser
//    sda::utils::CmdLineParser parser;
//
//    // Switches
//    //**************//"<Full Arg>",  "<Short Arg>", "<Description>", "<Default>"
//    parser.addSwitch("--xclbin_file", "-x", "input binary file string", "kv-hashmap.xcbin");
//    parser.addSwitch("--device", "-d", "device id", "1");
//    parser.parse(argc, argv);

    // Read settings
    auto binaryFile = "kv-store.xclbin";
    std::string dev_id = "1";

    auto device = xrt::device(dev_id);
    auto uuid = device.load_xclbin(binaryFile);
    auto krnl = xrt::kernel(device, uuid, "kv_store_top");

    auto kernel_bo = xrt::bo(device, BUFFER_SIZE * sizeof(Entry), krnl.group_id(0));
//    int* kernel_bo_map = kernel_bo.map<Entry *>();

    // init
    // for (int i = 0; i < VALUE_NUMS; i++) {
    // 	kernel_bo_map[i] = 0;
    // }

    // validation
    std::atomic<int32_t> index{0};
    int thread_nums = 20;
    std::vector<std::thread> threads(thread_nums);

    for (int i = 0; i < thread_nums; i++) {
        threads[i] = std::thread([i, &index, &kernel_bo, &krnl, &device] {
            auto result_bo = xrt::bo(device, 4, krnl.group_id(0));
            int* result_bo_map = result_bo.map<int *>();
            int op = 0;
            if (i % 2 == 1) {
              op = 1;
            }
            int next_index = ++index;
            Entry entry = {next_index, op, i / 2, i / 2 * 100};
            kv_store_apply(kernel_bo, result_bo, krnl, entry);
            std::cout << "Value for thread " << i << " op " << op << ": " << result_bo_map[0] << std::endl;
        });
    }

    for (int i = 0; i < thread_nums; i++) {
    	threads[i].join();
    }

    // Entry entry1 = {index, 0, 1, 100};
    // index++;
    // Entry entry2 = {index, 0, 2, 200};
    // index++;
    // kv_store_apply(kernel_bo, result_bo, krnl, entry1, &result);  // Insert (1, 100)
    // kv_store_apply(kernel_bo, result_bo, krnl, entry2, &result);  // Insert (2, 200)

    // // Get values
    // Entry entry3 = {index, 1, 1, 0};
    // index++;
    // kv_store_apply(kernel_bo, result_bo, krnl, entry3, &result);  // Get value for key 1
    // std::cout << "Value for key 1: " << result_bo_map[0] << std::endl;

    // Entry entry4 = {index, 1, 2, 0};
    // index++;
    // kv_store_apply(kernel_bo, result_bo, krnl, entry4, &result);  // Get value for key 2
    // std::cout << "Value for key 2: " << result_bo_map[0] << std::endl;

    //     // Delete a key-value pair
    // kv_store_apply(kernel_bo, result_bo, krnl, 2, 1, 0, &result);  // Delete key 1

    //     // Try to get the deleted key
    // kv_store_apply(kernel_bo, result_bo, krnl, 1, 1, 0, &result);  // Get value for key 1
    // std::cout << "Value for key 1 after deletion: " << result_bo_map[0] << std::endl;

    return 0;
}
