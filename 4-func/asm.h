#pragma once

#include "memory_io.h"
#include <vector>
#include <istream>
#include <map>

class assembler {
public:
  static constexpr uint32_t MEM_SIZE = 1024;

  assembler(std::istream &i);

  std::vector<unsigned char> run() &&;

private:
  struct assembler_writer : public memory_io {
    std::vector<std::pair<std::string, unsigned char *>> fixup_labels;

    using memory_io::write;
    void write(const std::string &val, label);
    void write(int val, reg);
  };

  std::vector<unsigned char> mem = std::vector<unsigned char>(MEM_SIZE);
  assembler_writer writer{mem.data()};
  std::map<std::string, size_t> label2offset;
  std::istream &i;

  bool one();
};