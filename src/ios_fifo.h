#ifndef __IOS_FIFO_H
#define __IOS_FIFO_H

#ifdef __cplusplus
extern "C" {
#endif


typedef enum
{
    AM_IOSTEST_SLAVE_STATE_NODATA   = 0,
    AM_IOSTEST_SLAVE_STATE_DATA     = 1,
} AM_IOSTEST_SLAVE_STATE_E;

extern volatile AM_IOSTEST_SLAVE_STATE_E g_iosState;
extern void *g_pIOSHandle;

void inform_host(void);

#ifdef __cplusplus
}
#endif

#endif /* __IOS_FIFO_H */
