#pragma once

#include "io.h"

struct memory {
  static constexpr size_t REG_SIZE = 16;
  static constexpr size_t STACK_SIZE = 6000;
  intptr_t reg_file[REG_SIZE] {};
  unsigned char stack_mem[STACK_SIZE] = {};
  size_t stack = STACK_SIZE;

  uintptr_t alloc(size_t sz);
  intptr_t &deref(uintptr_t addr);
  int simRand();
  void simFlush();
  void simPutPixel(int x, int y, int c);
  void simClear(int c);
};

struct memory_reader : ram_io {
  memory &cpu;

  using ram_io::read;
  auto &read(reg) {
    return cpu.reg_file[ram_io::read(reg{})];
  }
  auto &read(reg_ptr) {
    return *(intptr_t *)&cpu.stack_mem[read(reg{})];
  }
};
