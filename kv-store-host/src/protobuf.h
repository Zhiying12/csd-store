#include <cstdint>
#include <string>

class Command {
 public:
  Command() {}
  Command(int64_t type, int64_t key, int64_t value)
      : type_(type), key_(key), value_(value) {}

  int64_t type_ = -1;
  int64_t key_;
  int64_t value_;
};

class Instance {
 public:
  Instance() {}
  Instance(int64_t ballot, int64_t index, int64_t client_id, 
           int64_t type, int64_t key, int64_t value)
      : ballot_(ballot), 
        index_(index), 
        client_id_(client_id), 
        command_(type, key, value) {}

 public:
  Command command_;
  int64_t ballot_;
  int64_t index_;
  int64_t client_id_;
  // int state_;
};
// }  // namespace multipaxos
