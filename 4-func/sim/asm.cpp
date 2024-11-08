#include "../memory_io.h"
#include "../util.h"
#include "../isa.h"
#include "asm.h"

assembler::assembler(std::istream &i) : i(i) {}

struct assembler_reader {
  std::istream &s;

  int read(reg) {
    int val;
    char r;
    s >> std::ws >> r;
    if (r != 'r') {
      std::cerr << "Expected register argument" << std::endl;
      exit(1);
    }
    s >> val;
    return val;
  }

  template<typename T>
  T read(imm<T>) {
    T val;
    s >> val;
    return val;
  }

  std::string read(label) {
    std::string name;
    s >> name;
    return name;
  }
};

void assembler::assembler_writer::write(const std::string &val, label) {
  fixup_labels.emplace_back(val, mem);
  memory_io::write(uint32_t{0}, label{});
}

void assembler::assembler_writer::write(int val, reg) {
  memory_io::write((uint8_t)val, reg{});
}

bool assembler::one() {
  std::string inst;
  i >> inst;

  std::cout << "inst: " << inst << std::endl;

  assembler_reader reader(i);

  if (inst.ends_with(":")) {
    label2offset[inst.substr(0, inst.size() - 1)] = writer.mem - mem.data();
    return true;
  }

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
  while (one())
    ;

  mem.resize(writer.mem - mem.data());

  for (auto &&[label, at] : writer.fixup_labels) {
    writer.mem = at;
    writer.memory_io::write((uint32_t)label2offset[label], label);
  }

  return std::move(mem);
}
