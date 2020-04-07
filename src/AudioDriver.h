#ifndef __AUDIODRIVER_H
#define __AUDIODRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "am_mcu_apollo.h"

#define BUF_SIZE			128

extern volatile bool g_bPDMDataReady;
extern int16_t g_i16PDMBuf[2][BUF_SIZE];
extern uint32_t g_u32PDMPingpong;

void pdm_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __AUDIODRIVER_H */

