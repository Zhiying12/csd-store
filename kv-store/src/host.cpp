#include "cmdlineparser.h"
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

#include "protobuf.h"

int BUFFER_SIZE = 4096;

void kv_store_apply(xrt::bo& store_bo, xrt::bo& log_bo, xrt::bo& result_bo, xrt::kernel& krnl, int index) {
    auto run = krnl(store_bo, log_bo, result_bo, index, BUFFER_SIZE);
    run.wait();
    result_bo.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
}

int main(int argc, char** argv) {
    // Command Line Parser
    sda::utils::CmdLineParser parser;

    // Switches
    //**************//"<Full Arg>",  "<Short Arg>", "<Description>", "<Default>"
    parser.addSwitch("--xclbin_file", "-x", "input binary file string", "kv-store.xcbin");
    parser.addSwitch("--device_id", "-d", "device id", "0");
    parser.addSwitch("--buffer", "-b", "buffer size", "64");
    parser.addSwitch("--number", "-n", "number of instances", "64");
    parser.parse(argc, argv);

    // Read settings
    auto binaryFile = parser.value("xclbin_file");
    std::string dev_id = parser.value("device_id");
    BUFFER_SIZE = stoi(parser.value("buffer"));
    auto size = stoi(parser.value("number"));

    auto device = xrt::device(dev_id);
    auto uuid = device.load_xclbin(binaryFile);
    auto krnl = xrt::kernel(device, uuid, "kv_store_find");

    auto log_bo = xrt::bo(device, BUFFER_SIZE * sizeof(Instance), krnl.group_id(0));
    auto result_bo = xrt::bo(device, BUFFER_SIZE * sizeof(Instance), krnl.group_id(1));
    auto log_bo_map = log_bo.map<Instance *>();
    auto result_bo_map = result_bo.map<Instance *>();

    for (int i = 0; i < BUFFER_SIZE; i++) {
    	Instance inst;
    	log_bo_map[i] = inst;
    	result_bo_map[i] = inst;
    }

    for (int i = 0; i < size; i++) {
        log_bo_map[i].command_.value_ = i;
    }

    std::cout << result_bo_map[size - 1].command_.value_ << "\n";
    log_bo.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    std::cout << "sync to device\n";
    auto run = krnl(log_bo, result_bo, size);
    run.wait();
    std::cout << "run completion\n";
    result_bo.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    std::cout << "sync from device\n";
    std::cout << result_bo_map[size - 1].command_.value_ << "\n";


    // validation
    // std::atomic<int32_t> index{0};
    // int thread_nums = 20;
    // std::vector<std::thread> threads(thread_nums);

    // for (int i = 0; i < thread_nums; i++) {
    //     threads[i] = std::thread([i, &index, &log_bo, &store_bo, &krnl, &store_krnl, &device] {
    //         int op = 0;
    //         int next_index = ++index;
    //         Entry entry = {next_index, op, i / 2, i / 2 * 100};
    //         insert_log_entry(log_bo, krnl, entry);
            
    //         auto result_bo = xrt::bo(device, 4, krnl.group_id(0));
    //         int* result_bo_map = result_bo.map<int *>();
    //         kv_store_apply(store_bo, log_bo, result_bo, store_krnl, next_index);
    //         std::cout << "Value for thread " << i << " op " << op << ": " << result_bo_map[0] << std::endl;
    //     });
    // }

    // for (int i = 0; i < thread_nums; i++) {
    // 	threads[i].join();
    // }

    return 0;
}
