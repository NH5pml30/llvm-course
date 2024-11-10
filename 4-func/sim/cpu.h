#pragma once

#include "../memory.h"

struct CPU : memory {
  static constexpr uint32_t CODE_SIZE = 1024;
  uint32_t PC = 0;
  unsigned char code[CODE_SIZE] = {};

  CPU();

  void stop();
  void step();
  void jump(uint32_t to);
};

