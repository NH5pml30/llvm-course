#include "../io.h"
#include "../util.h"
#include "../isa.h"
#include "asm.h"

assembler::assembler(std::istream &i) : i(i) {}

void assembler::assembler_writer::write(const std::string &val, label) {
  fixup_labels[mem] = val;
  byte_io::write(word_t{0}, label{});
}

void assembler::assembler_writer::write(int val, reg) {
  byte_io::write((uint8_t)val, reg{});
}

bool assembler::one() {
  std::string inst;
  i >> inst;

  std::cout << "inst: " << inst << std::endl;

  if (inst.ends_with(":")) {
    label2offset[inst.substr(0, inst.size() - 1)] = writer.mem - mem.data();
    return true;
  }

  assembler_reader reader(i);

  bool found = find_inst<bool>(
      [&inst]<typename Inst>(Inst) { return Inst::name == inst; },
      [&]<typename Inst>(Inst) {
        auto arg_values = read_args_storage(reader, typename Inst::args_t{});
        writer.write<uint8_t>(Inst::op_code);
        arg_values.write(writer);
        return true;
      });
  if (found)
    return true;

  if (!inst.empty()) {
    std::cerr << "Unknown instruction: " << inst << "\n";
    exit(1);
  }

  return false;
}

std::vector<unsigned char> assembler::run() && {
  // read insts
  while (one())
    ;

  mem.resize(writer.mem - mem.data());

  // resolve labels
  for (auto &&[at, label] : writer.fixup_labels) {
    writer.mem = at;
    writer.byte_io::write(label2offset[label]);
  }

  return std::move(mem);
}
