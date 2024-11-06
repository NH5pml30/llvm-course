#pragma once

#include <cstdint>
#include <cstdlib>

struct CPU {
  static constexpr uint32_t REG_SIZE = 16;
  static constexpr uint32_t MEM_SIZE = 256;
  static constexpr uint32_t STACK_SIZE = 256;
  uint32_t next_PC;
  uint32_t PC = 0;
  uint32_t reg_file[REG_SIZE] {};
  unsigned char mem[MEM_SIZE] = {};
  unsigned char stack_mem[STACK_SIZE] = {};
  uint32_t stack = STACK_SIZE;

  CPU();

  void stop();
  void step();
};
