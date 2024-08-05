//#include "cmdlineparser.h"
//#include "xcl2.hpp"
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
// XRT includes
#include "experimental/xrt_bo.h"
#include "experimental/xrt_device.h"
#include "experimental/xrt_kernel.h"

int MAX_NUMS = 1024;
int STRUCT_FIELDS = 3;
int VALUE_NUMS = MAX_NUMS * STRUCT_FIELDS;
int MAX_BUFFER_SIZE = VALUE_NUMS * 4;

void kv_store_apply(xrt::bo& kernel_bo, xrt::bo& result_bo, xrt::kernel& krnl, int op, int key, int value, int* result) {
	auto run = krnl(kernel_bo, result_bo, op, key, value, VALUE_NUMS);
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

    auto kernel_bo = xrt::bo(device, MAX_BUFFER_SIZE, krnl.group_id(0));
    auto result_bo = xrt::bo(device, 4, krnl.group_id(1));
    int* kernel_bo_map = kernel_bo.map<int *>();
    int* result_bo_map = result_bo.map<int *>();

    // init
    for (int i = 0; i < VALUE_NUMS; i++) {
    	kernel_bo_map[i] = 0;
    }
    result_bo_map[0] = 1;

    // testing
    int result = 0;
    kv_store_apply(kernel_bo, result_bo, krnl, 0, 1, 100, &result);  // Insert (1, 100)
    kv_store_apply(kernel_bo, result_bo, krnl, 0, 2, 200, &result);  // Insert (2, 200)

    kv_store_apply(kernel_bo, result_bo, krnl, 4, 1, 0, &result);
    std::cout << "Value for index 0 after insertion: " << result_bo_map[0] << std::endl;

        // Get values
    kv_store_apply(kernel_bo, result_bo, krnl, 1, 1, 0, &result);  // Get value for key 1
    std::cout << "Value for key 1: " << result_bo_map[0] << std::endl;

    kv_store_apply(kernel_bo, result_bo, krnl, 1, 2, 0, &result);  // Get value for key 2
    std::cout << "Value for key 2: " << result_bo_map[0] << std::endl;

        // Delete a key-value pair
    kv_store_apply(kernel_bo, result_bo, krnl, 2, 1, 0, &result);  // Delete key 1

        // Try to get the deleted key
    kv_store_apply(kernel_bo, result_bo, krnl, 1, 1, 0, &result);  // Get value for key 1
    std::cout << "Value for key 1 after deletion: " << result_bo_map[0] << std::endl;

    return 0;
}
