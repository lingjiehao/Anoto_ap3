#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include  <limits.h>

#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"

#include "audiodriver.h"
#include "ios_fifo.h"
#include "SBC.h"

#include "FreeRTOS.h"
#include "task.h"

//*****************************************************************************
//
// Audio task handle.
//
//*****************************************************************************
TaskHandle_t audio_task_handle;

//*****************************************************************************
//
// Short Description.
//
//*****************************************************************************
void
AudioTask(void *pvParameters)
{
	uint32_t numWritten = 0;
	uint32_t ui32UsedSpace = 0;
	uint32_t u32PDMpg;
	uint32_t ulNotifiedValue;

	audio_task_handle = xTaskGetCurrentTaskHandle();

	SBC_init();
    //
    // Loop forever.
    //
    while(1)
    {

		xTaskNotifyWait( 0x00,      /* Don't clear any notification bits on entry. */
				ULONG_MAX, /* Reset the notification value to 0 on exit. */
				&ulNotifiedValue, /* Notified value pass out in ulNotifiedValue. */
				portMAX_DELAY );  /* Block indefinitely. */
		

		u32PDMpg = g_u32PDMPingpong;

		//am_hal_gpio_out_bit_clear(8);
#ifdef RELAJET	
		NR_process(g_i16PDMBuf[(u32PDMpg-1)%2]);
#endif
		SBC_process((int8_t*)(g_i16PDMBuf[(u32PDMpg-1)%2]));
		//am_hal_gpio_out_bit_set(8);
		
		am_hal_ios_fifo_write(g_pIOSHandle, (uint8_t *)g_i16PDMBuf[(u32PDMpg-1)%2], BUF_SIZE*2/4, &numWritten);

        // If we were Idle - need to inform Host if there is new data
        if (g_iosState == AM_IOSTEST_SLAVE_STATE_NODATA)
        {
            am_hal_ios_fifo_space_used(g_pIOSHandle, &ui32UsedSpace);
            if (ui32UsedSpace)
            {
                g_iosState = AM_IOSTEST_SLAVE_STATE_DATA;
                inform_host();
            }
        }
        
       
    }
}

