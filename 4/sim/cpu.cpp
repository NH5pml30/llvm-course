#include "cpu.h"
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

void CPU::step() {
  //std::cout << "inst: " << #name << "\n";
  regs_stack_reader reader{&code[PC], *this};
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

void CPU::jump(word_t to) {
  this->PC = to;
}
