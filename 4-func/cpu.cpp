#include "cpu.h"
#include "memory_io.h"

extern "C" {
#include "sim.h"
}

CPU::CPU() {
  simInit();
}

void CPU::stop() {
  simExit();
  exit(0);
}

void CPU::step() {
  memory_io mem_io{&mem[PC]};
  unsigned char op = mem_io.read<unsigned char>();
  CPU *cpu = this;
#define ISA(opcode, name, exec, args_type)                                               \
  case opcode: {                                                                         \
    auto arg_values = read_args_storage(mem_io, argument_type<void(args_type)>::type{}); \
    next_PC = mem_io.mem - mem;                                                          \
    exec;                                                                                \
    break;                                                                               \
  }
  switch (op) {
#include "isa.h"
  }
#undef ISA
  PC = next_PC;
}
