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
#include <limits.h>
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
#include "timer.h"
#include "timer_if.h"


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

// RED : Speed up (1) - bonus
// BLUE : Slow Down (1) - bonus
// MAGENTA : Demon (1) - every 10 seconds
// CYAN : Exorcism (1) - 5 seconds after demon
// GREEN : Score double (1) - ultra rare bonus
// YELLOW : Score increment (3) - instant respawn

struct Spot{
    int x;
    int y;
    int color;
};

Spot spotsYellow[3];
Spot spotsBonus[2];
Spot spotBonusQueue;

long score;
float speed;

int gameOver;

int timeToSpawnGreen;
int timeToSpawnDemon;
int demonSpawned;
int timeToSpawnCure;

static void TimerIntHandler()
{
    Timer_IF_InterruptClear(TIMERA2_BASE);
    timerCount++;
}

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

int genColor(int rand){
    switch(rand){
        case 0:
            return MAGENTA;
        case 1:
            return RED;
        case 2:
            return BLUE;
        case 3: 
            return CYAN;
        case 4: 
            return GREEN;
        case 5: 
            return YELLOW; 
        default:
            break; 
    }
    return BLACK;
}

void spawnYellowSpot(int idx){
    spotsYellow[idx].x = rand() % 123;
    spotsYellow[idx].y = rand() % 123;
    if(spotsYellow[idx].x < 4){
        spotsYellow[idx].x = 4;
    }
    if(spotsYellow[idx].y < 4){
        spotsYellow[idx].y = 4;
    }
    spotsYellow[idx].color = YELLOW;
}

void spawnBonusSpot(int idx){
    spotsBonus[idx].x = rand() % 123;
    spotsBonus[idx].y = rand() % 123;
    if(timeToSpawnGreen == 1 && (rand() % 3) == 0){
        spotsBonus[idx].color = GREEN;
    }
    else{
        spotsBonus[idx].color = genColor((rand() % 2) + 1);
    }
    if(spotsBonus[idx].x < 4){
        spotsBonus[idx].x = 4;
    }
    if(spotsBonus[idx].y < 4){
        spotsBonus[idx].y = 4;
    }
}

void drawSpots(){
    int i = 0;
    for(; i < 3; i++){
        fillCircle(spotsYellow[i].x, spotsYellow[i].y, 4, spotsYellow[i].color)
    }
    for(i = 0; i < 2; i++){
        //fillCircle(spotsBonus[i].x, spotsBonus[i].y, 4, spotsBonus[i].color)
    }
    //fillCircle()
}

int collision(int x, int y, int ox, int oy){
    if((x >= ox - 4 && x <= ox + 4) && (y >= oy - 4 && y <= oy + 4)){
        return 1;
    }
    return 0;
}

void spotCollision(int x, int y){
    for(; i < 3; i++){
        if(collision(x, y, spotsYellow[i].x, spotsYellow[i].y)){
            score++;
            fillCircle(spotsYellow[i].x, spotsYellow[i].y, 4, BLACK);
            spawnYellowSpot(i);
        }
    }
    // for(i = 0; i < 2; i++){
    //     if(collision(x, y, spotsBonus[i].x, spotsBonus[i].y)){
    //         fillCircle(spotsBonus[i].x, spotsBonus[i].y, 4, BLACK);
    //         if(spotsBonus.color == RED){
    //             speed = speed * 2;
    //         }
    //         else if(spotsBonus.color == BLUE){
    //             speed = speed / 2;
    //         }
    //         else{
    //             score = score * 2;
    //         }
    //         spawnBonusSpot(i);
    //     }
    // }
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

    availToSpawnSpots = (Spot) malloc(sizeof(Spot) * 6);

    fillScreen(BLACK);
    int ox = 0, oy = 0;
    int radius = 4;
    timeToSpawnGreen = 0;
    timeToSpawnDemon = 0;
    timeToSpawnCure = 0;
    int i;
    for(i = 0; i < 3; i++){
        spawnYellowSpot(i);
    }
    for(i = 0; i < 2; i++){
        spawnBonusSpot(i);
    }

    gameOver = 0;
    ox = rand() % 122;
    oy = rand() % 122;
    if(ox < radius){
        ox = radius;
    }
    if(oy < radius){
        oy = radius;
    }
    printf("%d , %d\n", ox, oy);
    while(1){

        if(!gameOver){
            fillCircle((int) y, (int) x, radius, BLACK);//reset screen by filling circle back to black

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
            if(x > (127 - radius)){
                x = 127 - radius;
            }

            if(y > (127 - radius)){
                y = 127 - radius;
            }

            if(x < radius){
                x = radius;
            }

            if(y < radius){
                y = radius;
            }

            //draw circle in x and y value, switched because axis was flipped from what I originally planned
            spotCollision();
            fillCircle((int) y, (int) x, radius, WHITE);
            drawSpots();

        }

    }

    //
    // Disable chip select
    //
    //MAP_SPICSEnable(GSPI_BASE);
    //GPIOPinWrite(GPIOA1_BASE, 0x1, 0x0);



}

