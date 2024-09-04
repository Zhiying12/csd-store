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

#include "json.h"
#include "replicant.h"
using nlohmann::json;

int BUFFER_SIZE = 10000;
int STRUCT_FIELDS = 3;
int VALUE_NUMS = BUFFER_SIZE * STRUCT_FIELDS;
int MAX_BUFFER_SIZE = VALUE_NUMS * 4;

struct Entry {
    int index;
    int op;
    int key;
    int value;
};

void insert_log_entry(xrt::bo& log_bo, xrt::kernel& krnl, Entry& entry) {
    auto run = krnl(log_bo, entry, BUFFER_SIZE);
    if (run) {
      run.wait();
    } else
      std::cout << "false run\n";
}

void kv_store_apply(xrt::bo& store_bo, xrt::bo& log_bo, xrt::bo& result_bo, xrt::kernel& krnl, int index) {
    auto run = krnl(store_bo, log_bo, result_bo, index, MAX_BUFFER_SIZE);
    run.wait();
    result_bo.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
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
    auto krnl = xrt::kernel(device, uuid, "append_instance");
    auto store_krnl = xrt::kernel(device, uuid, "kv_store_top");

    auto log_bo = xrt::bo(device, BUFFER_SIZE * sizeof(Entry), krnl.group_id(0));
    auto store_bo = xrt::bo(device, MAX_BUFFER_SIZE, krnl.group_id(0));

    // init
    // for (int i = 0; i < VALUE_NUMS; i++) {
    // 	kernel_bo_map[i] = 0;
    // }

    std::ifstream f("config.json");
    json config;
    f >> config;
    config["id"] = 0;
    
    boost::asio::io_context io_context(config["threadpool_size"]);
    auto replicant = std::make_shared<Replicant>(&io_context, config);

    // validation
    std::atomic<int32_t> index{0};
    int thread_nums = 20;
    std::vector<std::thread> threads(thread_nums);

    for (int i = 0; i < thread_nums; i++) {
        threads[i] = std::thread([i, &index, &log_bo, &store_bo, &krnl, &store_krnl, &device] {
            int op = 0;
            int next_index = ++index;
            Entry entry = {next_index, op, i / 2, i / 2 * 100};
            insert_log_entry(log_bo, krnl, entry);
            
            auto result_bo = xrt::bo(device, 4, krnl.group_id(0));
            int* result_bo_map = result_bo.map<int *>();
            kv_store_apply(store_bo, log_bo, result_bo, store_krnl, next_index);
            std::cout << "Value for thread " << i << " op " << op << ": " << result_bo_map[0] << std::endl;
        });
    }

    for (int i = 0; i < thread_nums; i++) {
    	threads[i].join();
    }

    return 0;
}
