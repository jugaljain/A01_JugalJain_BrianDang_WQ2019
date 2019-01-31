//*****************************************************************************
//
// Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/ 
// 
// 
//  Redistribution and use in source and binary forms, with or without 
//  modification, are permitted provided that the following conditions 
//  are met:
//
//    Redistributions of source code must retain the above copyright 
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the 
//    documentation and/or other materials provided with the   
//    distribution.
//
//    Neither the name of Texas Instruments Incorporated nor the names of
//    its contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
//  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
//  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
//  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************

//*****************************************************************************
//
// Application Name     - SPI Demo
// Application Overview - The demo application focuses on showing the required 
//                        initialization sequence to enable the CC3200 SPI 
//                        module in full duplex 4-wire master and slave mode(s).
// Application Details  -
// http://processors.wiki.ti.com/index.php/CC32xx_SPI_Demo
// or
// docs\examples\CC32xx_SPI_Demo.pdf
//
//*****************************************************************************


//*****************************************************************************
//
//! \addtogroup SPI_Demo
//! @{
//
//*****************************************************************************

// Standard includes
#include <string.h>

// Driverlib includes
#include "hw_types.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "hw_ints.h"
#include "spi.h"
#include "rom.h"
#include "rom_map.h"
#include "utils.h"
#include "prcm.h"
#include "uart.h"
#include "interrupt.h"
#include "gpio.h"
#include "gpio_if.h"

// Common interface includes
#include "uart_if.h"
#include "pinmux.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1351.h"
#include "glcdfont.h"

#define APPLICATION_VERSION     "1.1.1"

// Color definitions
#define BLACK           0x0000
#define BLUE            0x001F
#define GREEN           0x07E0
#define CYAN            0x07FF
#define RED             0xF800
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define WHITE           0xFFFF

//*****************************************************************************
//
// Application Master/Slave mode selector macro
//
// MASTER_MODE = 1 : Application in master mode
// MASTER_MODE = 0 : Application in slave mode
//
//*****************************************************************************
#define MASTER_MODE      0

#define SPI_IF_BIT_RATE  100000
#define TR_BUFF_SIZE     100

#define MASTER_MSG       "This is CC3200 SPI Master Application\n\r"
#define SLAVE_MSG        "This is CC3200 SPI Slave Application\n\r"

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************


#if defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif
//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************



//*****************************************************************************
//
//! SPI Master mode main loop
//!
//! This function configures SPI modelue as master and enables the channel for
//! communication
//!
//! \return None.
//
//*****************************************************************************


//*****************************************************************************
//
//! SPI Slave mode main loop
//!
//! This function configures SPI modelue as slave and enables the channel for
//! communication
//!
//! \return None.
//
//*****************************************************************************

//*****************************************************************************
//
//! Board Initialization & Configuration
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************

static void
BoardInit(void)
{
/* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS
  //
  // Set vector table base
  //
#if defined(ccs)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif
#endif
    //
    // Enable Processor
    //
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}

//*****************************************************************************
//
//! Main function for spi demo application
//!
//! \param none
//!
//! \return None.
//
//*****************************************************************************
void main()
{
    //
    // Initialize Board configurations
    //
    BoardInit();

    //
    // Muxing UART and SPI lines.
    //
    PinMuxConfig();

    MAP_PRCMPeripheralClkEnable(PRCM_GSPI,PRCM_RUN_MODE_CLK);

        //
        // Initialising the Terminal.
        //
        InitTerm();

        //
        // Clearing the Terminal.
        //
        ClearTerm();

        //
        // Display the Banner
        //
        Message("\n\n\n\r");
        Message("\t\t   ********************************************\n\r");
        Message("\t\t        CC3200 SPI Demo Application  \n\r");
        Message("\t\t   ********************************************\n\r");
        Message("\n\n\n\r");

        //
        // Reset the peripheral
        //
        MAP_PRCMPeripheralReset(PRCM_GSPI);

    //
    // Initialize the message
    //
    //memcpy(g_ucTxBuff,MASTER_MSG,sizeof(MASTER_MSG));

    //
    // Set Tx buffer index
    //
//    ucTxBuffNdx = 0;
//    ucRxBuffNdx = 0;

    //
    // Reset SPI
    //
    MAP_SPIReset(GSPI_BASE);

    //
    // Configure SPI interface
    //
    MAP_SPIConfigSetExpClk(GSPI_BASE,MAP_PRCMPeripheralClockGet(PRCM_GSPI),
                     SPI_IF_BIT_RATE,SPI_MODE_MASTER,SPI_SUB_MODE_0,
                     (SPI_SW_CTRL_CS |
                     SPI_4PIN_MODE |
                     SPI_TURBO_OFF |
                     SPI_CS_ACTIVEHIGH |
                     SPI_WL_8));

    //
    // Enable SPI for communication
    //
    MAP_SPIEnable(GSPI_BASE);


    //
    // Send the string to slave. Chip Select(CS) needs to be
    // asserted at start of transfer and deasserted at the end.
    //
   // MAP_SPITransfer(GSPI_BASE,g_ucTxBuff,g_ucRxBuff,50,
    //        SPI_CS_ENABLE|SPI_CS_DISABLE);

   // unsigned long ulDummy;

    char* text = "Hello World";
    // Enable Chip select
    //

    //enable SPICS and OC to high
    MAP_SPICSEnable(GSPI_BASE);
    GPIOPinWrite(GPIOA1_BASE, 0x1, 0x1);
    //GPIOPinWrite(GPIOA0_BASE, 0x40, 0x00);
    //GPIOPinWrite(GPIOA0_BASE, 0x80, 0x00);

    //MAP_SPIDataGet(GSPI_BASE,&ulDummy);

    Adafruit_Init();
    int x; //x axis counter for displaying ASCII
    int y;//y axis counter for ASCII
    char c;//counter for the character
    while(1){
        //drawing ascii values
        fillScreen(BLACK);
        x =0;
        y =0;
        //code to cycle through and print ASCII characters
        for(c = 0; c <= 254; c++){
            drawChar(x, y, c, WHITE, BLACK, 1);
            x += 5;
            if(x >= 125){
                x = 0;
                y += 7;
            }
            if(y >= 125){
                y = 0;
                fillScreen(BLACK);
            }

        }
        delay(100);
        //hello world display
        setTextColor(WHITE, BLACK);
        setTextSize(1);
        Outstr(text);
        delay(100);
        //horizontal bands
        fillScreen(BLACK);
        drawLine(0, 0, 127, 0, WHITE);
        drawLine(0, 21, 127, 21, BLUE);
        drawLine(0, 42, 127, 42, GREEN);
        drawLine(0, 63, 127, 63, CYAN);
        drawLine(0, 84, 127, 84, RED);
        drawLine(0, 105, 127, 105, MAGENTA);
        drawLine(0, 127, 127, 127, YELLOW);
        delay(100);
        //vertical bands
        fillScreen(BLACK);
        drawLine(0, 0, 0, 127, WHITE);
        drawLine(21, 0, 21, 127, BLUE);
        drawLine(42, 0, 42, 127, GREEN);
        drawLine(63, 0, 63, 127, CYAN);
        drawLine(84, 0, 84, 127, RED);
        drawLine(105, 0, 105, 127, MAGENTA);
        drawLine(127, 0, 127, 127, YELLOW);
        fillScreen(BLACK);
        lcdTestPattern();
        fillScreen(BLACK);
        lcdTestPattern2();
        testlines(YELLOW);
        delay(100);
        testfastlines(BLUE, GREEN);
        delay(100);
        testdrawrects(GREEN);
        delay(100);
        testfillrects(YELLOW, MAGENTA);
        delay(100);
        testfillcircles(10, BLUE);
        testdrawcircles(10, WHITE);
        fillCircle(64, 64, 4, WHITE);
        delay(100);
        testroundrects();
        delay(100);
        testtriangles();
        delay(100);
    }

    //
    // Disable chip select
    //
    //MAP_SPICSEnable(GSPI_BASE);
    //GPIOPinWrite(GPIOA1_BASE, 0x1, 0x0);



}

