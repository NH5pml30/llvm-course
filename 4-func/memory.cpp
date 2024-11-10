#include "memory.h"

extern "C" {
#include "sim.h"
}

uintptr_t memory::alloc(size_t sz) {
  return stack -= sz;
}

intptr_t &memory::deref(uintptr_t addr) {
  return *(intptr_t *)&stack_mem[addr];
}

int memory::simRand() {
  return ::simRand();
}

void memory::simFlush() {
  return ::simFlush();
}

void memory::simPutPixel(int x, int y, int c) {
  return ::simPutPixel(x, y, c);
}

void memory::simClear(int c) {
  return ::simClear(c);
}
