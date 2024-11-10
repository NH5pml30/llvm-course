#include "memory.h"

extern "C" {
#include "sim.h"
}

uint32_t memory::alloc(uint32_t sz) {
  return stack -= sz;
}

int32_t &memory::deref(uint32_t addr) {
  return *(int32_t *)(&stack_mem[addr]);
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
