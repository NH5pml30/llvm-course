#pragma once

#include "io.h"

struct ctx_regs {
  static constexpr size_t REG_SIZE = 16;
  sword_t reg_file[REG_SIZE] {};

  int simRand();
  void simFlush();
  void simPutPixel(int x, int y, int c);
  void simClear(int c);
};

struct ctx_regs_stack : ctx_regs {
  static constexpr size_t STACK_SIZE = 6000;
  unsigned char stack_mem[STACK_SIZE] = {};
  size_t stack = STACK_SIZE;

  word_t alloc(size_t sz);
  sword_t &deref(word_t addr);
};

struct regs_stack_reader : byte_io {
  ctx_regs_stack &cpu;

  using byte_io::read;
  auto &read(reg) {
    return cpu.reg_file[byte_io::read(reg{})];
  }
  auto &read(reg_ptr) {
    return *(sword_t *)&cpu.stack_mem[read(reg{})];
  }
};
