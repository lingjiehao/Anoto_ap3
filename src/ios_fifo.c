//*****************************************************************************
//
//! @file ios_fifo.c
//! Pin connections for the I/O Master board to the I/O Slave board.
//! SPI:
//!     HOST (AP3 as MT2523)                    SLAVE (AP3)
//!     --------------------                    ----------------
//!     GPIO[10] GPIO Interrupt (slave to host) GPIO[4]  GPIO interrupt
//!     GPIO[5]  IOM0 SPI SCK                   GPIO[0]  IOS SPI SCK
//!     GPIO[7]  IOM0 SPI MOSI                  GPIO[1]  IOS SPI MOSI
//!     GPIO[6]  IOM0 SPI MISO                  GPIO[2]  IOS SPI MISO
//!     GPIO[11] IOM0 SPI nCE                   GPIO[3]  IOS SPI nCE
//!     GND                                     GND
//!
//
//*****************************************************************************

//*****************************************************************************
//
// Copyright (c) 2020, Ambiq Micro
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
//
// Third party software included in this distribution is subject to the
// additional license terms as defined in the /docs/licenses directory.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// This is part of revision 2.4.2 of the AmbiqSuite Development Package.
//
//*****************************************************************************

#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"

#include "audiodriver.h"
#include "sbc.h"
#include "RelaJet.h"
#include "ios_fifo.h"

#define  FW_VER	2

#define     TEST_IOS_XCMP_INT   1

#define     AM_HAL_IOS_INT_ERR  (AM_HAL_IOS_INT_FOVFL | AM_HAL_IOS_INT_FUNDFL | AM_HAL_IOS_INT_FRDERR)

#define AM_HAL_IOS_XCMP_INT (AM_HAL_IOS_INT_XCMPWR | AM_HAL_IOS_INT_XCMPWF | AM_HAL_IOS_INT_XCMPRR | AM_HAL_IOS_INT_XCMPRF)


typedef enum
{
    AM_IOSTEST_CMD_START_DATA    = 0,
    AM_IOSTEST_CMD_STOP_DATA     = 1,
    AM_IOSTEST_CMD_ACK_DATA      = 2,
} AM_IOSTEST_CMD_E;

#define AM_IOSTEST_IOSTOHOST_DATAAVAIL_INTMASK  1


volatile AM_IOSTEST_SLAVE_STATE_E g_iosState;

void *g_pIOSHandle;

//*****************************************************************************
//
// Message buffers.
//
// data from the IOS interface, which is only 8 bits wide.
//
//*****************************************************************************
#define AM_IOS_TX_BUFSIZE_MAX   1023
uint8_t g_pui8TxFifoBuffer[AM_IOS_TX_BUFSIZE_MAX];

//*****************************************************************************
//
// GPIO Configuration
//
//*****************************************************************************
const am_hal_gpio_pincfg_t g_AM_BSP_GPIO_ENABLE =
{
    .uFuncSel            = AM_HAL_PIN_4_SLINT,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_2MA,
    .eCEpol              = AM_HAL_GPIO_PIN_CEPOL_ACTIVEHIGH,
    .eGPOutcfg           = AM_HAL_GPIO_PIN_OUTCFG_OPENDRAIN,
};
const am_hal_gpio_pincfg_t g_AM_BSP_GPIO_DISABLE =
{
    .uFuncSel            = AM_HAL_PIN_4_GPIO,
    .eGPOutcfg           = AM_HAL_GPIO_PIN_OUTCFG_DISABLE,
};

//*****************************************************************************
//
// SPI Slave Configuration
//
//*****************************************************************************
static am_hal_ios_config_t g_sIOSSpiConfig =
{
    // Configure the IOS in SPI mode.
    .ui32InterfaceSelect = AM_HAL_IOS_USE_SPI,

    // Eliminate the "read-only" section, so an external host can use the
    // entire "direct write" section.
    .ui32ROBase = 0x78,

    // Making the "FIFO" section as big as possible.
    .ui32FIFOBase = 0x80,

    // We don't need any RAM space, so extend the FIFO all the way to the end
    // of the LRAM.
    .ui32RAMBase = 0x100,

    // FIFO Threshold - set to half the size
    .ui32FIFOThreshold = 0x20,

    .pui8SRAMBuffer = g_pui8TxFifoBuffer,
    .ui32SRAMBufferCap = AM_IOS_TX_BUFSIZE_MAX,
};

//*****************************************************************************
//
// Configure the SPI slave.
//
//*****************************************************************************
static void ios_set_up(void)
{

    // Configure SPI interface
    am_bsp_ios_pins_enable(0, AM_HAL_IOS_USE_SPI);
    //
    // Configure the IOS interface and LRAM structure.
    //
    am_hal_ios_initialize(0, &g_pIOSHandle);
    am_hal_ios_power_ctrl(g_pIOSHandle, AM_HAL_SYSCTRL_WAKE, false);
    am_hal_ios_configure(g_pIOSHandle, &g_sIOSSpiConfig);


    //
    // Clear out any IOS register-access interrupts that may be active, and
    // enable interrupts for the registers we're interested in.
    //
    am_hal_ios_interrupt_clear(g_pIOSHandle, AM_HAL_IOS_INT_ALL);
    am_hal_ios_interrupt_enable(g_pIOSHandle, AM_HAL_IOS_INT_ERR | AM_HAL_IOS_INT_FSIZE);
#ifdef TEST_IOINTCTL
    am_hal_ios_interrupt_enable(g_pIOSHandle, AM_HAL_IOS_INT_IOINTW);
#endif
#ifdef TEST_IOS_XCMP_INT
    am_hal_ios_interrupt_enable(g_pIOSHandle, AM_HAL_IOS_XCMP_INT);
#endif

    //
    // Set the bit in the NVIC to accept access interrupts from the IO Slave.
    //
    NVIC_EnableIRQ(IOSLAVE_IRQn);

    // Set up the IOSINT interrupt pin
    am_hal_gpio_pinconfig(4, g_AM_BSP_GPIO_ENABLE);

}


// Inform host of new data available to read
void inform_host(void)
{
    uint32_t ui32Arg = AM_IOSTEST_IOSTOHOST_DATAAVAIL_INTMASK;
    // Update FIFOCTR for host to read
    am_hal_ios_control(g_pIOSHandle, AM_HAL_IOS_REQ_FIFO_UPDATE_CTR, NULL);
    // Interrupt the host
    am_hal_ios_control(g_pIOSHandle, AM_HAL_IOS_REQ_HOST_INTSET, &ui32Arg);
}

//*****************************************************************************
//
// IO Slave Main ISR.
//
//*****************************************************************************
void am_ioslave_ios_isr(void)
{
    uint32_t ui32Status;
    uint8_t  *pui8Packet;
    uint32_t ui32UsedSpace = 0;

    //
    // Check to see what caused this interrupt, then clear the bit from the
    // interrupt register.
    //

    am_hal_ios_interrupt_status_get(g_pIOSHandle, false, &ui32Status);

    am_hal_ios_interrupt_clear(g_pIOSHandle, ui32Status);

    if (ui32Status & AM_HAL_IOS_INT_FUNDFL)
    {
        am_util_stdio_printf("Hitting underflow for the requested IOS FIFO transfer\n");
        // We should never hit this case unless the threshold has beeen set
        // incorrect, or we are unable to handle the data rate
        // ERROR!
        am_hal_debug_assert_msg(0,
            "Hitting underflow for the requested IOS FIFO transfer.");
    }

    if (ui32Status & AM_HAL_IOS_INT_ERR)
    {
        // We should never hit this case
        // ERROR!
        am_hal_debug_assert_msg(0,
            "Hitting ERROR case.");
    }

    if (ui32Status & AM_HAL_IOS_INT_FSIZE)
    {
        //
        // Service the I2C slave FIFO if necessary.
        //
        am_hal_ios_interrupt_service(g_pIOSHandle, ui32Status);
    }

    if (ui32Status & AM_HAL_IOS_INT_XCMPWR)
    {
        //
        // Set up a pointer for writing 32-bit aligned packets through the IO slave
        // interface.
        //
        pui8Packet = (uint8_t *) am_hal_ios_pui8LRAM;
        switch(pui8Packet[0])
        {
            case AM_IOSTEST_CMD_START_DATA:
				pdm_init();
                break;

            case AM_IOSTEST_CMD_STOP_DATA:

                g_iosState = AM_IOSTEST_SLAVE_STATE_NODATA;
                break;

            case AM_IOSTEST_CMD_ACK_DATA:
                // Host done reading the last block signalled
                // Check if any more data available
                am_hal_ios_fifo_space_used(g_pIOSHandle, &ui32UsedSpace);
                if (ui32UsedSpace)
                {
                    g_iosState = AM_IOSTEST_SLAVE_STATE_DATA;
                    inform_host();
                }
                else
                {
                    g_iosState = AM_IOSTEST_SLAVE_STATE_NODATA;
                }
                break;

            default:
                break;
        }
    }
}

static void VersionConfig(void)
{
#ifdef RELAJET
	int16_t ret;
	int16_t LibVer;

	/*return 1 means Licence key verification is OK */
	ret = getVersion(&LibVer);
	if(ret == 1)
		*(int16_t *)(am_hal_ios_pui8LRAM+6) = LibVer;// Relajet NR Version
	else
		*(int16_t *)(am_hal_ios_pui8LRAM+6) = -1;// Licence key verification failed

#else
	*(int16_t *)(am_hal_ios_pui8LRAM+6) = 0;//Without Relajet NR
#endif
	*(int16_t *)(am_hal_ios_pui8LRAM+4) = FW_VER;

}

#if 1
//*****************************************************************************
//
// Main function.
//
//*****************************************************************************
int main(void)
{
    am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_SYSCLK_MAX, 0);

    //
    // Set the default cache configuration
    //
    am_hal_cachectrl_config(&am_hal_cachectrl_defaults);
    am_hal_cachectrl_enable();

    //
    // Configure the board for low power operation.
    //
    am_bsp_low_power_init();

    //
    // Enable the ITM print interface.
    //
    am_bsp_itm_printf_enable();

    //
    // Clear the terminal and print the banner.
    //
    am_util_stdio_terminal_clear();
	
	//am_util_stdio_printf("IOS FIFO Example\n");

	
	
    //
    // Enable the IOS
    //
    ios_set_up();
    VersionConfig();

    //
    // Enable interrupts so we can receive messages from the boot host.
    //
    am_hal_interrupt_master_enable();


	//
    // Run the application.
    //
    run_tasks();

    //
    // We shouldn't ever get here.
    //
    while (1)
    {
    }

}
#else
//*****************************************************************************
//
// Main function.
//
//*****************************************************************************
int main(void)
{
    am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_SYSCLK_MAX, 0);

    //
    // Set the default cache configuration
    //
    am_hal_cachectrl_config(&am_hal_cachectrl_defaults);
    am_hal_cachectrl_enable();

    //
    // Configure the board for low power operation.
    //
    am_bsp_low_power_init();

    //
    // Enable the ITM print interface.
    //
    am_bsp_itm_printf_enable();

    //
    // Clear the terminal and print the banner.
    //
    am_util_stdio_terminal_clear();
	
	//am_util_stdio_printf("IOS FIFO Example\n");

	SBC_init();
	
    //
    // Enable the IOS
    //
    ios_set_up();
    VersionConfig();

    //
    // Enable interrupts so we can receive messages from the boot host.
    //
    am_hal_interrupt_master_enable();

    //
    // Loop forever.
    //
    while(1)
    {
		uint32_t numWritten = 0;
		uint32_t ui32UsedSpace = 0;
		uint32_t u32PDMpg;
		
		uint32_t ui32IntStatus = am_hal_interrupt_master_disable();
		if (g_bPDMDataReady)
		{
			g_bPDMDataReady = false;
			u32PDMpg = g_u32PDMPingpong;
			// Enable the interrupts
			am_hal_interrupt_master_set(ui32IntStatus);
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
        else
        {
            //
            // Go to Deep Sleep.
            //
            am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_NORMAL);
            // Enable the interrupts
            am_hal_interrupt_master_set(ui32IntStatus);
        }
    }
}
#endif

