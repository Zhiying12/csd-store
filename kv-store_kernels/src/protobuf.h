#include <cstdint>
#include <string>

class Command {
 public:
  inline Command() {}

  int64_t type_;
  int64_t key_;
  int64_t value_;
};

class Instance {
 public:
  Instance() {}

 public:
  Command command_;
  int64_t ballot_ = 0;
  int64_t index_;
  int64_t client_id_;
  // int state_;
};
// }  // namespace multipaxos
