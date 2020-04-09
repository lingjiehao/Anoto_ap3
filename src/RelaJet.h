#ifndef __RELAJET_H
#define __RELAJET_H

#ifdef __cplusplus
extern "C" {
#endif

#include "am_mcu_apollo.h"


/*
LibVer is an int16_t IN/OUT pointor
return 1 means Licence key verification is OK 
*/
int16_t getVersion(int16_t *LibVer);

/*
buffer is a float INOUT pointor to float[128] buffer array
*/
void relajet_nr_go(float *buffer);

/*
i16Buffer is a int16 INOUT pointor to int16[128] buffer array
*/
void NR_process(int16_t *i16Buffer);

#ifdef __cplusplus
}
#endif

#endif /* __RELAJET_H */

