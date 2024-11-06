#pragma once

#include "util.h"

struct memory_io {
  unsigned char *mem;

  template<typename T>
  T read() {
    auto value = *(T *)mem;
    mem += sizeof(T);
    return value;
  }

  template<typename T>
  void write(T val) {
    *(T *)mem = val;
    mem += sizeof(T);
  }

  template<typename T>
  auto read(imm<T>) {
    return read<T>();
  }

  auto read(reg) {
    return read<uint8_t>();
  }

  auto read(label) {
    return read<uint32_t>();
  }

  template<typename T, typename Ig>
  void write(T val, Ig) {
    write<T>(val);
  }
};