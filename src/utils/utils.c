#include <utils.h>
#include <stdlib.h>
#include "../common/types.h"

int rand_uniform_int(int min, int max){
    // Returns a number in [min, max]
    return (min + rand() % (min-max +1)); 
}

float rand_uniform_float(void){
    return (float)rand() /(float) RAND_MAX;
}

int should_drop(float prob){
    return rand_uniform_float()<prob;
}
uint32_t calc_tx_delay(int length){
    // Delay =(L*8*1000)/C_BPS     msec
    return (uint32_t) (((uint32_t)length* 8UL *1000UL) /C_BPS );
}
