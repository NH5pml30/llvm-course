#include "memory.h"

extern "C" {
#include "sim.h"
}

uintptr_t ctx_regs_stack::alloc(size_t sz) {
  return stack -= sz;
}

intptr_t &ctx_regs_stack::deref(uintptr_t addr) {
  return *(intptr_t *)&stack_mem[addr];
}

int ctx_regs::simRand() {
  return ::simRand();
}

void ctx_regs::simFlush() {
  return ::simFlush();
}

void ctx_regs::simPutPixel(int x, int y, int c) {
  return ::simPutPixel(x, y, c);
}

void ctx_regs::simClear(int c) {
  return ::simClear(c);
}
