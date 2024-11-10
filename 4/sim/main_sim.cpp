#include <fstream>
#include <sstream>

#include "cpu.h"
#include "asm.h"

int main() {
  std::ifstream in("../app.s");
  /* std::string source = R"__delim(
entry:
  ADDi r0 r0 1
  WRITE r0
  JMP entry
)__delim";
  std::stringstream in(source); */

  assembler a(in);
  auto data = std::move(a).run();
  CPU cpu;
  std::copy(data.begin(), data.end(), cpu.code);
  while (true) {
    cpu.step();
  }
  return 0;
}
