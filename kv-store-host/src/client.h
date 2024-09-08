#ifndef CLIENT_H_
#define CLIENT_H_

#include <boost/asio.hpp>
#include <memory>
#include <unordered_map>

class MultiPaxos;
class ClientManager;

class Client : public std::enable_shared_from_this<Client> {
 public:
  Client(int64_t id,
         boost::asio::ip::tcp::socket socket,
         MultiPaxos* multi_paxos,
         ClientManager* manager)
      : id_(id),
        socket_(std::move(socket)),
        multi_paxos_(multi_paxos),
        manager_(manager) {}
  Client(Client const&) = delete;
  Client& operator=(Client const&) = delete;
  Client(Client const&&) = delete;
  Client& operator=(Client const&&) = delete;

  void Start();
  void Stop();

  void Read();
  void Write(std::string const& response);

 private:
  int64_t id_;
  boost::asio::ip::tcp::socket socket_;
  boost::asio::streambuf request_;
  boost::asio::streambuf response_;
  MultiPaxos* multi_paxos_;
  ClientManager* manager_;
};

#endif
