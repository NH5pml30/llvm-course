#include "sim/sim.h"

int main(void) {
  simInit();
  app();
  simExit();
  return 0;
}