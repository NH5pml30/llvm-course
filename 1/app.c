#include "sim.h"

#define N_STARS 200
#define WORLD_SIZE_X 100
#define WORLD_SIZE_Y 100
#define WORLD_SIZE_Z 100

void generateStar(int *s) {
  s[0] = simRand() % (2 * WORLD_SIZE_X) - WORLD_SIZE_X;
  s[1] = simRand() % (2 * WORLD_SIZE_Y) - WORLD_SIZE_Y;
  s[2] = WORLD_SIZE_Z;
}

void app() {
  int stars[N_STARS][3] = {};

  for (int i = 0; i < N_STARS; i++) {
    generateStar(stars[i]);
    stars[i][2] = simRand() % WORLD_SIZE_Z;
  }

  while (1) {
    simClear(0x00000000);
    for (int i = 0; i < N_STARS; i++) {
      stars[i][2]--;
      if (stars[i][2] <= 0)
        generateStar(stars[i]);
      int scr_x = SIM_X_SIZE / 2 + (SIM_X_SIZE / 2) * stars[i][0] / stars[i][2];
      int scr_y = SIM_Y_SIZE / 2 + (SIM_Y_SIZE / 2) * stars[i][1] / stars[i][2];
      if (scr_x >= 0 && scr_x < SIM_X_SIZE && scr_y >= 0 && scr_y < SIM_Y_SIZE)
        simPutPixel(scr_x, scr_y, 0xFFFFFFFF);
    }
    simFlush();
  }
}