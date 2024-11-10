#pragma once

#include <istream>
#include <map>
#include <vector>

#include "../io.h"

class assembler {
public:
  static constexpr uint32_t MEM_SIZE = 1024;

  assembler(std::istream &i);

  std::vector<unsigned char> run() &&;

private:
  struct assembler_writer : public ram_io {
    std::map<unsigned char *, std::string> fixup_labels;

    using ram_io::write;
    void write(const std::string &val, label);
    void write(int val, reg);
  };

  std::vector<unsigned char> mem = std::vector<unsigned char>(MEM_SIZE);
  assembler_writer writer{mem.data()};
  std::map<std::string, size_t> label2offset;
  std::istream &i;

  bool one();
};