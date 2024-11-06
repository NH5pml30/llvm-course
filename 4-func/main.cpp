#include <sstream>

#include "asm.h"
#include "cpu.h"

int main() {
  std::string src = R"__delim(
app:
  ALLOCA r0 2400

init_header:
  JMPEQi r1 200 init_exit

  MULi r3 r1 12
  ADD r2 r0 r3

  SIM_RAND r3
  REMi r3 r3 200
  ADDi r3 r3 -100
  STORE r2 r3

  SIM_RAND r3
  REMi r3 r3 200
  ADDi r3 r3 -100
  ADDi r2 r2 4
  STORE r2 r3

  SIM_RAND r3
  REMi r3 r3 100
  ADDi r2 r2 4
  STORE r2 r3
  ADDi r1 r1 1
  JMP init_header

init_exit:
  SETi r1 0
  SIM_CLEAR r1

update_header:
  JMPEQi r1 200 update_exit

  MULi r3 r1 12
  ADD r2 r0 r3
  SET r3 r2
  ADDi r3 r3 8
  LOAD r4 r3
  ADDi r4 r4 -1
  STORE r3 r4
  CLTi r5 r4 1
  JMPZ r5 compute

  SIM_RAND r4
  REMi r4 r4 200
  ADDi r4 r4 -100
  SET r6 r2
  STORE r6 r4

  SIM_RAND r4
  REMi r4 r4 200
  ADDi r4 r4 -100
  ADDi r6 r6 4
  STORE r6 r4

  ADDi r6 r6 4
  SETi r4 100
  STORE r6 r4

compute:
  LOAD r5 r2
  SHLi r5 r5 8
  DIV r5 r5 r4
  ADDi r5 r5 256

  ADDi r6 r2 4
  LOAD r6 r6
  SHLi r6 r6 7
  DIV r6 r6 r4
  ADDi r6 r6 128

  CLTi r7 r5 0
  JMPNZ r7 update_end
  CGTi r7 r5 511
  JMPNZ r7 update_end
  CLTi r7 r6 0
  JMPNZ r7 update_end
  CGTi r7 r6 255
  JMPNZ r7 update_end

  SETi r7 -1
  SIM_PUT_PIXEL r5 r6 r7

update_end:
  ADDi r1 r1 1
  JMP update_header

update_exit:
  SIM_FLUSH
  JMP init_exit
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
