#include "contexts.h"

extern "C" {
#include "sim.h"
}

word_t ctx_regs_stack::alloc(size_t sz) {
  return stack -= sz;
}

sword_t &ctx_regs_stack::deref(word_t addr) {
  return *(sword_t *)&stack_mem[addr];
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
