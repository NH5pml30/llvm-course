#pragma once

#include "../memory.h"

struct CPU : memory {
  static constexpr size_t CODE_SIZE = 2048;
  uintptr_t PC = 0;
  unsigned char code[CODE_SIZE] = {};

  CPU();

  void stop();
  void step();
  void jump(uintptr_t to);
};

