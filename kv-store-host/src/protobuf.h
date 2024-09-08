#ifndef PROTOBUF_H
#define PROTOBUF_H

#include <cstdint>
#include <cstring>
#include <string>

const int KEY_SIZE = 23;
const int VALUE_SIZE = 500;

class Command {
 public:
  Command() {}
  Command(int64_t type, int64_t key, int64_t value)
      : type_(type) {}
  
  Command(int64_t type, std::string const& key, std::string const& value)
      : type_(type) {
    std::strcpy(key_, key.c_str());
    std::strcpy(value_, value.c_str());
  }

  int64_t type_ = -1;
  char key_[KEY_SIZE + 1];
  char value_[VALUE_SIZE + 1];
};

class Instance {
 public:
  Instance() {}
  Instance(int64_t ballot, int64_t index, int64_t client_id, 
           int64_t type, std::string const& key, std::string const& value)
      : command_(type, key, value),
        ballot_(ballot), 
        index_(index), 
        client_id_(client_id) {}

 public:
  Command command_;
  int64_t ballot_;
  int64_t index_;
  int64_t client_id_;
  int64_t state_ = 0;
};

#endif
