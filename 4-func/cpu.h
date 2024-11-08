#pragma once

#include <cstdint>
#include <cstdlib>

struct CPU {
  struct memory {
    static constexpr uint32_t REG_SIZE = 16;
    static constexpr uint32_t STACK_SIZE = 3000;
    int32_t reg_file[REG_SIZE] {};
    unsigned char stack_mem[STACK_SIZE] = {};
    uint32_t stack = STACK_SIZE;
  };
  memory data;
  static constexpr uint32_t CODE_SIZE = 1024;
  uint32_t PC = 0;
  unsigned char code[CODE_SIZE] = {};

  CPU();

  void stop();
  void step();
  void jump(uint32_t to);
  uint32_t alloc(uint32_t sz);
  int32_t &deref(uint32_t addr);
  int simRand();
  void simFlush();
  void simPutPixel(int x, int y, int c);
  void simClear(int c);
};
