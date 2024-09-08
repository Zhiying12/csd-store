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
      partition_size_(config["partition_size"]) {
  if (config["log"] == "xrt") {
    auto binaryFile = "kv-store.xclbin";
    std::string dev_id = "1";
    device_ = xrt::device(dev_id);
    auto uuid = device_.load_xclbin(binaryFile);
    for (auto i = 0; i < partition_size_; i++) {
      logs_.emplace_back(CreateLog(i, device_, uuid, config["store"]));
    }
  } else {
    for (auto i = 0; i < partition_size_; i++) {
      logs_.emplace_back(CreateLog(i, kvstore::CreateStore(config), config["store"]));
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
//   client_manager_.StopAll();
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
  for (;;) {
    int64_t id, result;
    std::tie(id, result) = logs_[index]->Execute();
    if (id == -1)
      break;
    // auto [id, result] = std::move(*r);
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
