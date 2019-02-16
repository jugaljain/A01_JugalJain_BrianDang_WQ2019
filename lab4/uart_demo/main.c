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
// Application Name     - UART Demo
// Application Overview - The objective of this application is to showcase the 
//                        use of UART. The use case includes getting input from 
//                        the user and display information on the terminal. This 
//                        example take a string as input and display the same 
//                        when enter is received.
// Application Details  -
// http://processors.wiki.ti.com/index.php/CC32xx_UART_Demo_Application
// or
// docs\examples\CC32xx_UART_Demo_Application.pdf
//
//*****************************************************************************

//*****************************************************************************
//
//! \addtogroup uart_demo
//! @{
//
//*****************************************************************************

// Driverlib includes
#include "rom.h"
#include "rom_map.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "hw_types.h"
#include "hw_ints.h"
#include "uart.h"
#include "interrupt.h"
#include "pinmux.h"
#include "utils.h"
#include "prcm.h"
#include "gpio.h"
#include "spi.h"
#include "timer.h"

// Common interface include
#include "uart_if.h"
#include "timer_if.h"

//*****************************************************************************
//                          MACROS                                  
//*****************************************************************************
#define APPLICATION_VERSION  "1.1.1"
#define APP_NAME             "UART Echo"
#define CONSOLE              UARTA0_BASE
#define UartGetChar()        MAP_UARTCharGet(CONSOLE)
#define UartPutChar(c)       MAP_UARTCharPut(CONSOLE,c)
#define MAX_STRING_LENGTH    80

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
volatile int g_iCounter = 0;

#if defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif
//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************

int coeff[8] = { 31548, 31281, 30950, 30556, 29143, 28360, 27409, 26258 }; // array to store the calculated coefficients
int f_tone[8] = { 697, 770, 852, 941, 1209, 1336, 1477, 1633 }; // frequencies of rows & columns
volatile int samples[410];
volatile int count;             // samples count
volatile bool flag;             // flag set when the samples buffer is full with N samples
volatile bool new_dig;          // flag set when inter-digit interval (pause) is detected
int power_all[8];
//*****************************************************************************
//                      LOCAL DEFINITION                                   
//*****************************************************************************

//*****************************************************************************
//
//! Application startup display on UART
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
static void DisplayBanner(char * AppName)
{

    Report("\n\n\n\r");
    Report("\t\t *************************************************\n\r");
    Report("\t\t        CC3200 %s Application       \n\r", AppName);
    Report("\t\t *************************************************\n\r");
    Report("\n\n\n\r");
}

static void TimerIntHandler()
{
    Timer_IF_InterruptClear(TIMERA2_BASE);
    timerCount++;
}

static void TimerIntSampleHandler()
{
    unsigned long* ADC;
    Timer_IF_InterruptClear(TIMERA2_BASE);
    GPIOPinWrite(GPIOA0_BASE, 0x1, 0x0); //turn ADC CS to low
    SPIDataGet(GSPI_BASE, ADC);

    if (count < 410){
        samples[count++] = *ADC>>2;

    }
    else if (count == 410){
        flag = 1;
    }
    GPIOPinWrite(GPIOA0_BASE, 0x1, 0x1);

}

//-------Goertzel function---------------------------------------//
long int goertzel(int sample[], long int coeff, int N)
//---------------------------------------------------------------//
{
//initialize variables to be used in the function
    int Q, Q_prev, Q_prev2,i;
    long prod1,prod2,prod3,power;

    Q_prev = 0;         //set delay element1 Q_prev as zero
    Q_prev2 = 0;        //set delay element2 Q_prev2 as zero
    power=0;            //set power as zero

    for (i=0; i<N; i++) // loop N times and calculate Q, Q_prev, Q_prev2 at each iteration
        {
            Q = (sample[i]) + ((coeff* Q_prev)>>14) - (Q_prev2); // >>14 used as the coeff was used in Q15 format
            Q_prev2 = Q_prev;                                    // shuffle delay elements
            Q_prev = Q;
        }

        //calculate the three products used to calculate power
        prod1=( (long) Q_prev*Q_prev);
        prod2=( (long) Q_prev2*Q_prev2);
        prod3=( (long) Q_prev *coeff)>>14;
        prod3=( prod3 * Q_prev2);

        power = ((prod1+prod2-prod3))>>8; //calculate power using the three products and scale the result down

        return power;
}
//*****************************************************************************
//
//! Board Initialization & Configuration
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
static void BoardInit(void)
{
    /* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS
    //
    // Set vector table base
    //
#if defined(ccs)
    MAP_IntVTableBaseSet((unsigned long) &g_pfnVectors[0]);
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
//! Main function handling the uart echo. It takes the input string from the
//! terminal while displaying each character of string. whenever enter command 
//! is received it will echo the string(display). if the input the maximum input
//! can be of 80 characters, after that the characters will be treated as a part
//! of next string.
//!
//! \param  None
//!
//! \return None
//! 
//*****************************************************************************
void main()
{
    char cString[MAX_STRING_LENGTH + 1];
    char cCharacter;
    int iStringLength = 0;
    //
    // Initailizing the board
    //
    BoardInit();
    //
    // Muxing for Enabling UART_TX and UART_RX.
    //
    PinMuxConfig();

    MAP_PRCMPeripheralClkEnable(PRCM_GSPI, PRCM_RUN_MODE_CLK);

    g_ulBase = TIMERA2_BASE;

    outMessage = (char*) malloc(sizeof(char) * 512);
    int i = 0;
    for (i = 0; i < 512; i++)
    {
        outMessage[i] = '\0';
    }
    //Report("Test1\n\r");
    UARTIntRegister(UARTA1_BASE, UARTIntHandler);
    UARTIntClear(UARTA1_BASE, UART_INT_RX);
    UARTIntEnable(UARTA1_BASE, UART_INT_RX);
    UARTFIFOEnable(UARTA1_BASE);
    UARTFIFOLevelSet(UARTA1_BASE, UART_FIFO_TX1_8, UART_FIFO_RX1_8);
    UARTConfigSetExpClk(UARTA1_BASE, PRCMPeripheralClockGet(PRCM_UARTA1),
                        115200, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                        UART_CONFIG_PAR_NONE));
    //UARTEnable(UARTA1_BASE);
    //Report("Test2\n\r");
    //
    // Register the interrupt handlers
    //
    //MAP_GPIOIntRegister(GPIOA3_BASE, GPIOA1IntHandler);
    TimerIntRegister(TIMERA2_BASE, TIMER_A, TimerIntHandler);
    TimerIntRegister(TIMERA2_BASE, TIMER_B, TimerIntSampleHandler);
    TimerConfigure(TIMERA2_BASE,
                   (TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PERIODIC_UP | TIMER_CFG_B_PERIODIC_UP));

    //
    // Configure rising edge interrupts on SW2 and SW3
    //
//    MAP_GPIOIntTypeSet(GPIOA3_BASE, 0x40, GPIO_BOTH_EDGES); // SW3
//    ulStatus = MAP_GPIOIntStatus(GPIOA3_BASE, false);
//    MAP_GPIOIntClear(GPIOA3_BASE, ulStatus);       // clear interrupts on GPIOA1

    // clear global variables
    SW2_intcount = 0;
    SW3_intcount = 0;
    SW2_intflag = 0;
    SW3_intflag = 0;
    long wStart = 0;
    timerCount = 0;

    MAP_PRCMPeripheralReset(PRCM_GSPI);
    MAP_SPIReset(GSPI_BASE);

    //
    // Configure SPI interface
    //
    MAP_SPIConfigSetExpClk(GSPI_BASE, MAP_PRCMPeripheralClockGet(PRCM_GSPI),
                           400000,
                           SPI_MODE_MASTER,
                           SPI_SUB_MODE_0, (SPI_SW_CTRL_CS |
                           SPI_4PIN_MODE |
                           SPI_TURBO_OFF |
                           SPI_CS_ACTIVEHIGH |
                           SPI_WL_8));

    //
    // Enable SPI for communication
    //
    MAP_SPIEnable(GSPI_BASE);

    //enable SPICS and OC to high
    MAP_SPICSEnable(GSPI_BASE);
    GPIOPinWrite(GPIOA1_BASE, 0x1, 0x1);
    Adafruit_Init();

    // Enable SW2 and SW3 interrupts
    //MAP_GPIOIntEnable(GPIOA3_BASE, 0x40);

    //    Timer_IF_Init(PRCM_TIMERA0, g_ulBase, TIMER_CFG_PERIODIC, TIMER_A, 0);
    //    Timer_IF_IntSetup(g_ulBase, TIMER_A, TimerIntHandler);
    TimerLoadSet(TIMERA2_BASE, TIMER_A, 0x1F40);
    TimerIntEnable(TIMERA2_BASE, TIMER_TIMA_TIMEOUT);

    TimerEnable(TIMERA2_BASE, TIMER_A);

    TimerLoadSet(TIMERA2_BASE, TIMER_B, 0x1388);
    TimerIntEnable(TIMERA2_BASE, TIMER_TIMB_TIMEOUT);

    TimerEnable(TIMERA2_BASE, TIMER_B);
    GPIOPinWrite(GPIOA0_BASE, 0x1, 0x1);

    while (1)
    {
        //GPIOPinWrite(GPIOA0_BASE, 0x1, 0x0);
        count=0;  //rest count
              flag=0;   //reset flag

            while(flag==0); // wait till N samples are read in the buffer and the flag set by the ADC ISR

                {
                    for (i=0;i<8;i++)
                        power_all[i]=goertzel(samples, coeff[i], N); // call goertzel to calculate the power at each frequency and store it in the power_all array


                }

            }
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

