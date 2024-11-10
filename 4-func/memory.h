#pragma once

#include "io.h"

struct memory {
  static constexpr uint32_t REG_SIZE = 16;
  static constexpr uint32_t STACK_SIZE = 3000;
  int32_t reg_file[REG_SIZE] {};
  unsigned char stack_mem[STACK_SIZE] = {};
  uint32_t stack = STACK_SIZE;

  uint32_t alloc(uint32_t sz);
  int32_t &deref(uint32_t addr);
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
    return *(uint32_t *)&cpu.stack_mem[read(reg{})];
  }
};
