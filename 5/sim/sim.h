#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define SIM_X_SIZE 512
#define SIM_Y_SIZE 256

#ifndef __sim__
void simFlush(void);
void simPutPixel(int x, int y, int argb);
void simClear(int argb);
int simRand(void);
#endif

extern void simInit(void);
extern void app(void);
extern void simExit(void);

#ifdef __cplusplus
}
#endif
