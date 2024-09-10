#include <cstdint>
#include <string>

class Command {
 public:
  Command() {}

  int64_t type_;
  // char key_[32];
  // char value_[512];
  int64_t key_ = 12345678;
  int64_t value_ = 87654321;
};

class Instance {
 public:
  Instance() {}

 public:
  Command command_;
  int64_t ballot_ = 0;
  int64_t index_ = 0;
  int64_t client_id_ = 0;
  int64_t state_ = 0;
};
