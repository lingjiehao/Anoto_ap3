#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"

#include "audiodriver.h"
#include "audio_task.h"

#include "FreeRTOS.h"
#include "task.h"



//#define OSR					(192) //over sampling rate 3MHz
//#define OSR				(128) //over sampling rate 2MHz
//#define OSR				(96) //over sampling rate 1.5MHz
//#define OSR				(64) //over sampling rate 1MHz
#define OSR				(48) //over sampling rate 750KHz
//#define OSR				(32) //over sampling rate 500KHz
//#define OSR				(24) //over sampling rate 350KHz
//#define OSR				(16) //over sampling rate 250KHz


//*****************************************************************************
//
// Global variables.
//
//*****************************************************************************
volatile bool g_bPDMDataReady = false;

int16_t g_i16PDMBuf[2][BUF_SIZE] = {{0},{0}};

uint32_t g_u32PDMPingpong = 0;


//*****************************************************************************
//
// PDM configuration information.
//
//*****************************************************************************
void *PDMHandle;

am_hal_pdm_config_t g_sPdmConfig =
{
	.eClkDivider = AM_HAL_PDM_MCLKDIV_1,
	.eLeftGain = AM_HAL_PDM_GAIN_P225DB,
	.eRightGain = AM_HAL_PDM_GAIN_P225DB,
	.ui32DecimationRate = (OSR/2),
	.bHighPassEnable = 0,
	.ui32HighPassCutoff = 0xB,
	.ePDMClkSpeed = AM_HAL_PDM_CLK_750KHZ,
	//.ePDMClkSpeed = AM_HAL_PDM_CLK_1_5MHZ,
	.bInvertI2SBCLK = 0,
	.ePDMClkSource = AM_HAL_PDM_INTERNAL_CLK,
	//.ePDMClkSource = AM_HAL_PDM_I2S_CLK,
	.bPDMSampleDelay = 0,
	.bDataPacking = 1,
	.ePCMChannels = AM_HAL_PDM_CHANNEL_LEFT,
	.bLRSwap = 0,
};

//*****************************************************************************
//
// Start a transaction to get some number of bytes from the PDM interface.
//
//*****************************************************************************
void
pdm_data_get(int16_t *dest)
{
	//
	// Configure DMA and target address.
	//
	am_hal_pdm_transfer_t sTransfer;
	sTransfer.ui32TargetAddr = (uint32_t ) dest;
	sTransfer.ui32TotalCount = BUF_SIZE*2;


	//
	// Start the data transfer.
	//
	am_hal_pdm_dma_start(PDMHandle, &sTransfer);

}

//*****************************************************************************
//
// PDM initialization.
//
//*****************************************************************************
void
pdm_init(void)
{
	//
	// Initialize, power-up, and configure the PDM.
	//
	am_hal_pdm_initialize(0, &PDMHandle);
	am_hal_pdm_power_control(PDMHandle, AM_HAL_PDM_POWER_ON, false);
	am_hal_pdm_configure(PDMHandle, &g_sPdmConfig);
	am_hal_pdm_enable(PDMHandle);

	//
	// Configure the necessary pins.
	//
	am_hal_gpio_pincfg_t sPinCfg = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	sPinCfg.uFuncSel = AM_HAL_PIN_12_PDMCLK;
	am_hal_gpio_pinconfig(12, sPinCfg);

	sPinCfg.uFuncSel = AM_HAL_PIN_11_PDMDATA;
	am_hal_gpio_pinconfig(11, sPinCfg);

	am_hal_pdm_fifo_flush(PDMHandle);

	
	//
	// Configure and enable PDM interrupts (set up to trigger on DMA
	// completion).
	//
	am_hal_pdm_interrupt_enable(PDMHandle, (AM_HAL_PDM_INT_DERR
	                                        | AM_HAL_PDM_INT_DCMP
	                                        | AM_HAL_PDM_INT_UNDFL
	                                        | AM_HAL_PDM_INT_OVF));

	NVIC_EnableIRQ(PDM_IRQn);
	NVIC_SetPriority(PDM_IRQn, NVIC_configMAX_SYSCALL_INTERRUPT_PRIORITY);


	// Start the first DMA transaction.
	pdm_data_get(g_i16PDMBuf[0]);

}

//*****************************************************************************
//
// PDM interrupt handler.
//
//*****************************************************************************
void
am_pdm0_isr(void)
{
	uint32_t ui32Status;
	BaseType_t xHigherPriorityTaskWoken;
	//am_hal_gpio_state_write(8 , AM_HAL_GPIO_OUTPUT_CLEAR);

	//
	// Read the interrupt status.
	//
	am_hal_pdm_interrupt_status_get(PDMHandle, &ui32Status, false);
	am_hal_pdm_interrupt_clear(PDMHandle, ui32Status);

	if (ui32Status & AM_HAL_PDM_INT_DCMP)
	{
		//g_bPDMDataReady = true;
		pdm_data_get(g_i16PDMBuf[(++g_u32PDMPingpong)%2]);
		xHigherPriorityTaskWoken = pdFALSE;
		xTaskNotifyFromISR(audio_task_handle, 1, eSetBits, &xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
 	//am_hal_gpio_output_toggle(8); 
}



