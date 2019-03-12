//*****************************************************************************
// Students:Brian Dang, Jugal Jain
//*****************************************************************************
//
// Application Name     - Acceleramator
// Application Overview - Reads acceleramotor data for X and Y and moves a circle arround the OLED display
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
#include <stdlib.h>
#include <stdio.h>
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
#include "i2c_if.h"

#define APPLICATION_VERSION     "1.1.1"
#define UART_PRINT              Report

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
    float x = 64;
    float y = 64;
    //int r = 4;
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

        I2C_IF_Open(I2C_MASTER_MODE_FST);//enable I2C

        unsigned char XBuf[256];//buffer to store X input
        unsigned char YBuf[256];//buffer to store Y input
        unsigned char temp[256];//temp for 2s complement conversion
        unsigned char ucRegOffsetX = 0x3;//X offset
        unsigned char ucRegOffsetY = 0x5;//Y offset


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

    // Enable Chip select
    //

    MAP_SPICSEnable(GSPI_BASE);
    GPIOPinWrite(GPIOA1_BASE, 0x1, 0x1);
    //GPIOPinWrite(GPIOA0_BASE, 0x40, 0x00);
    //GPIOPinWrite(GPIOA0_BASE, 0x80, 0x00);

    //MAP_SPIDataGet(GSPI_BASE,&ulDummy);

    //oled initialization and setting background to black
    Adafruit_Init();
    fillScreen(BLACK);
    int ox = 0, oy = 0;
    ox = rand() % 122;
    oy = rand() % 122;
    if(ox < 6){
        ox = 6;
    }
    if(oy < 6){
        oy = 6;
    }
    printf("%d , %d\n", ox, oy);
    while(1){
            fillCircle((int) y, (int) x, 4, BLACK);//reset screen by filling circle back to black

            //get x values
            I2C_IF_Write(0x18,&ucRegOffsetX,1,0);
            I2C_IF_Read(0x18, &XBuf[0], 1);

            //get y value from I2C
            I2C_IF_Write(0x18,&ucRegOffsetY,1,0);
            I2C_IF_Read(0x18, &YBuf[0], 1);

            //UART_PRINT("X: 0x%x, Y: 0x%x \n ", XBuf[0], YBuf[0]);//testing to see right values in terminal

            //if X negative number reverse back to positive and subtract
            if(XBuf[0] & 0x80){
                temp[0] = XBuf[0];
                temp[0] = temp[0] - 0x1;
                temp[0] = ~temp[0];
                x = x - (temp[0] / 2);//divide by 8 to slow down movement
            }
            else{//else add
                x = x + (XBuf[0] / 2);
            }

            //if Y is negative reverse the 2s complement back to positive and subtract
            if(YBuf[0] & 0x80){
                temp[0] = YBuf[0];
                temp[0] = temp[0] - 0x1;
                temp[0] = ~temp[0];
                y = y - (temp[0] / 2);
            }
            else{//else add
                y = y + (YBuf[0] / 2);
            }

            //checking for bounds
            if(x > 123){
                x = 123;
            }

            if(y > 123){
                y = 123;
            }

            if(x < 4){
                x = 4;
            }

            if(y < 4){
                y = 4;
            }

            //draw circle in x and y value, switched because axis was flipped from what I originally planned
            fillCircle((int) y, (int) x, 4, WHITE);

            fillCircle(ox, oy, 4, RED);
            if((y >= ox - 4 && y <= ox + 4) && (x >= oy - 4 && x <= oy + 4)){
                fillCircle(ox, oy, 4, BLACK);
                ox =  (rand() % 122);
                oy =  (rand() % 122);
                fillCircle(ox, oy, 4, RED);
            }

        }

    //
    // Disable chip select
    //
    //MAP_SPICSEnable(GSPI_BASE);
    //GPIOPinWrite(GPIOA1_BASE, 0x1, 0x0);



}

