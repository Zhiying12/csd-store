#include <cstdint>
#include <string>

class Command {
 public:
  Command() {}

  int64_t type_;
  char key_[32];
  char value_[512];
};

class Instance {
 public:
  Instance() {}

 public:
  Command command_;
  int64_t ballot_;
  int64_t index_;
  int64_t client_id_;
  int64_t state_ = 0;
};
