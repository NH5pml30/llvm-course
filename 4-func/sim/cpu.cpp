#include "../cpu.h"
#include "../memory_io.h"
#include "../isa.h"

extern "C" {
#include "../sim.h"
}

CPU::CPU() {
  simInit();
}

void CPU::stop() {
  simExit();
  exit(0);
}

struct cpu_reader : memory_io {
  CPU &cpu;

  using memory_io::read;
  auto &read(reg) {
    return cpu.data.reg_file[memory_io::read(reg{})];
  }
};

void CPU::step() {
  //std::cout << "inst: " << #name << "\n";                                             
  cpu_reader reader{&code[PC], *this};
  unsigned char op = reader.read<unsigned char>();
  find_inst([op]<typename Inst>(Inst) { return Inst::op_code == op; },
            [&]<typename Inst>(Inst) {
              auto arg_values =
                  read_args_storage(reader, typename Inst::args_t{});
              PC = reader.mem - code;
              std::apply(
                  []<typename... Ts>(Ts &&...args) {
                    Inst::exec(std::forward<Ts>(args)...);
                  },
                  std::tuple_cat(std::tuple<CPU &>(*this), arg_values.tuple()));
            });
}

void CPU::jump(uint32_t to) {
  this->PC = to;
}

uint32_t CPU::alloc(uint32_t sz) {
  return data.stack -= sz;
}

int32_t &CPU::deref(uint32_t addr) {
  return *(int32_t *)(&data.stack_mem[addr]);
}

int CPU::simRand() {
  return ::simRand();
}

void CPU::simFlush() {
  return ::simFlush();
}

void CPU::simPutPixel(int x, int y, int c) {
  return ::simPutPixel(x, y, c);
}

void CPU::simClear(int c) {
  return ::simClear(c);
}
