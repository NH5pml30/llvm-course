#include <sstream>

#include "asm.h"
#include "cpu.h"

int main() {
  std::string src = R"__delim(
entry:
  SETi r0 0
  SIM_CLEAR r0
  SETi r3 100
begin:
  SIM_FLUSH
  SIM_RAND r0
  REMi r0 r0 512
  SIM_RAND r1
  REMi r1 r1 256
  SETi r2 -1
  SIM_PUT_PIXEL r0 r1 r2
  ADDi r3 r3 -1
  JMPnz r3 begin
  EXIT
)__delim";
  std::istringstream ss(src);

  assembler a(ss);
  auto data = std::move(a).run();
  CPU cpu;
  std::copy(data.begin(), data.end(), cpu.mem);
  while (true) {
    cpu.step();
  }
  return 0;
}
