#pragma once

#include "../memory.h"

struct CPU : ctx_regs_stack {
  static constexpr size_t CODE_SIZE = 2048;
  word_t PC = 0;
  unsigned char code[CODE_SIZE] = {};

  CPU();

  void stop();
  void step();
  void jump(word_t to);
};

