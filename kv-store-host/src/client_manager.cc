#include "client_manager.h"
// #include <glog/logging.h>

using boost::asio::ip::tcp;

void ClientManager::Start(tcp::socket socket) {
  auto id = NextClientId();
  auto client =
      std::make_shared<Client>(id, std::move(socket), multi_paxos_, this);
  {
    std::unique_lock<std::mutex> lock(mu_);
    clients_.insert({id, client});
    // CHECK(ok);
  }
  // DLOG(INFO) << " client_manager started client " << id;
  client->Start();
}

client_ptr ClientManager::Get(int64_t id) {
  std::unique_lock<std::mutex> lock(mu_);
  auto it = clients_.find(id);
  if (it == clients_.end())
    return nullptr;
  return it->second;
}

void ClientManager::Stop(int64_t id) {
  std::unique_lock<std::mutex> lock(mu_);
  // DLOG(INFO) << " client_manager stopped client " << id;
  auto it = clients_.find(id);
  if (it == clients_.end()) {
    return;
  }
  // CHECK(it != clients_.end());
  it->second->Stop();
  clients_.erase(it);
}

void ClientManager::StopAll() {
  std::unique_lock<std::mutex> lock(mu_);
  for (auto& client : clients_) {
    // DLOG(INFO) << " client_manager stopping all clients " << id;
    client.second->Stop();
  }
  clients_.clear();
}
