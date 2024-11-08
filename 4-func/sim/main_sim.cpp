#include <fstream>

#include "../cpu.h"
#include "asm.h"

int main() {
  std::ifstream in("../app.s");

  assembler a(in);
  auto data = std::move(a).run();
  CPU cpu;
  std::copy(data.begin(), data.end(), cpu.code);
  while (true) {
    cpu.step();
  }
  return 0;
}
