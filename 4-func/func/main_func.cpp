#include <fstream>

#include "../cpu.h"
#include "../util.h"

extern "C" {
#include "../sim.h"
}

CPU::memory data;

struct func_reader {
  template<typename T>
  T read(imm<T>);
  int32_t &read(reg);
  uint32_t read(label);
};

void stop() {}

uint32_t next_PC;

#include "../isa.h"

int main() {
  std::ifstream in("../app.s");

  // static_assert(std::is_same_v<inst_enum_t, void>);

  return 0;
}