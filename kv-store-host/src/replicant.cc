// #include <glog/logging.h>
#include <boost/asio.hpp>
#include <condition_variable>
#include <cstring>
#include <iostream>
#include <memory>
#include <optional>

#include "json.h"
#include "replicant.h"

using boost::asio::ip::tcp;
using nlohmann::json;

Replicant::Replicant(boost::asio::io_context* io_context, json const& config)
    : id_(config["id"]),
      multi_paxos_(logs_, config),
      ip_port_(config["peers"][id_]),
      io_context_(io_context),
      acceptor_(boost::asio::make_strand(*io_context_)),
      client_manager_(id_, config["peers"].size(), &multi_paxos_),
      device_count_(config["device_count"]),
      partition_size_(config["partition_size"]),
      context_(device_count_),
      queue_(device_count_) {
  if (config["log"] == "xrt") {
    auto devices = xcl::get_xil_devices();
    std::string binaryFile = config["binary_file"];
    auto fileBuf = xcl::read_binary_file(binaryFile);
    cl::Program::Binaries bins{{fileBuf.data(), fileBuf.size()}};
    cl_int err;
    int partition_per_device = partition_size_ / device_count_;
    for (auto i = 0; i < device_count_; i++) {
      auto device = devices[i];
      context_[i] = cl::Context(device, nullptr, nullptr, nullptr, &err);
      queue_[i] = cl::CommandQueue(context_[i], device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &err);
      cl::Program program(context_[i], {device}, bins, nullptr, &err);
      for (auto j = 0; j < partition_per_device; j++) {
        logs_.emplace_back(CreateLog(j, i, context_[i], program, queue_[i], config["store"]));
      }
    }
  } else {
    for (auto i = 0; i < partition_size_; i++) {
      logs_.emplace_back(CreateLog(i, i%2, kvstore::CreateStore(config), config["store"]));
    }
  }
}

void Replicant::Start() {
  multi_paxos_.Start();
  StartExecutorThread();
  StartServer();
}

void Replicant::Stop() {
  StopServer();
  StopExecutorThread();
  multi_paxos_.Stop();
}

void Replicant::StartServer() {
  auto pos = ip_port_.find(":") + 1;
//   CHECK_NE(pos, std::string::npos);
  int port = std::stoi(ip_port_.substr(pos)) + 1;

  tcp::endpoint endpoint(tcp::v4(), port);
  acceptor_.open(endpoint.protocol());
  acceptor_.set_option(tcp::acceptor::reuse_address(true));
  acceptor_.set_option(tcp::no_delay(true));
  acceptor_.bind(endpoint);
  acceptor_.listen(5);
//   DLOG(INFO) << id_ << " starting server at port " << port;

  auto self(shared_from_this());
  boost::asio::dispatch(acceptor_.get_executor(), [this, self] { AcceptClient(); });
}

void Replicant::StopServer() {
  auto self(shared_from_this());
  boost::asio::post(acceptor_.get_executor(), [this, self] { acceptor_.close(); });
  client_manager_.StopAll();
}

void Replicant::StartExecutorThread() {
//   DLOG(INFO) << id_ << " starting executor thread";
  for (auto i = 0; i < partition_size_; i++) {
    executor_threads_.emplace_back(&Replicant::ExecutorThread, this, i);
  }
}

void Replicant::StopExecutorThread() {
//   DLOG(INFO) << id_ << " stopping executor thread";
  for (auto& log : logs_) {
    log->Stop();
  }
  for (auto& t : executor_threads_) {
    t.join();
  }
}

void Replicant::ExecutorThread(int index) {
  int64_t id;
  std::string result;
  for (;;) {
    std::tie(id, result) = logs_[index]->Execute();
    if (id == -1)
      break;
    auto client = client_manager_.Get(id);
    if (client)
      client->Write(result);
  }
}

void Replicant::AcceptClient() {
  auto self(shared_from_this());
  acceptor_.async_accept(boost::asio::make_strand(*io_context_),
                         [this, self](std::error_code ec, tcp::socket socket) {
                           if (!acceptor_.is_open())
                             return;
                           if (!ec) {
                             client_manager_.Start(std::move(socket));
                             AcceptClient();
                           }
                         });
}
