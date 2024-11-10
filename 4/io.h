#pragma once

#include "util.h"
#include <cstdint>
#include <iostream>
#include <istream>

struct byte_io {
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
    return read<word_t>();
  }

  template<typename T>
  void write(T val, imm<T>) {
    write<T>(val);
  }

  void write(uint8_t val, reg) {
    write<uint8_t>(val);
  }

  void write(word_t val, label) {
    write<word_t>(val);
  }
};

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
