#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

int rand_uniform_int(int min, int max);
float rand_uniform_float(void);
int should_drop(float prob);
uint32_t calc_tx_delay(int length);

#endif
