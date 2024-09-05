#ifndef REPLICANT_H_
#define REPLICANT_H_

#include <boost/asio.hpp>
#include <memory>

#include "client_manager.h"
#include "json_fwd.h"
#include "log.h"
// #include "memkvstore.h"
#include "multipaxos.h"

class Replicant : public std::enable_shared_from_this<Replicant> {
 public:
  Replicant(boost::asio::io_context* io_context, nlohmann::json const& config);
  Replicant(Replicant const& replicant) = delete;
  Replicant& operator=(Replicant const& replicant) = delete;
  Replicant(Replicant&& replicant) = delete;
  Replicant& operator=(Replicant&& replicant) = delete;

  void Start();
  void Stop();

 private:
  void StartServer();
  void StopServer();

  void StartExecutorThread();
  void StopExecutorThread();

  void ExecutorThread(int index);

  void AcceptClient();

  int64_t id_;
  std::vector<Log*> logs_;
  MultiPaxos multi_paxos_;
  std::string ip_port_;
  boost::asio::io_context* io_context_;
  boost::asio::ip::tcp::acceptor acceptor_;
  ClientManager client_manager_;
  std::vector<std::thread> executor_threads_;
  int partition_size_;
  xrt::device device_;
};

#endif
