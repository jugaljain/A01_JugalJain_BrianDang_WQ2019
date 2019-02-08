//*****************************************************************************
//
// Application Name     - int_sw
// Application Overview - The objective of this application is to demonstrate
//                          GPIO interrupts using SW2 and SW3.
//                          NOTE: the switches are not debounced!
//
//*****************************************************************************

//****************************************************************************
//
//! \addtogroup int_sw
//! @{
//
//****************************************************************************

// Standard includes
#include <stdio.h>

// Driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "interrupt.h"
#include "hw_apps_rcm.h"
#include "prcm.h"
#include "rom.h"
#include "rom_map.h"
#include "prcm.h"
#include "gpio.h"
#include "utils.h"
#include "timer.h"

// Common interface includes
#include "uart_if.h"
#include "timer_if.h"
#include "pin_mux_config.h"


//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
extern void (* const g_pfnVectors[])(void);

volatile unsigned long SW2_intcount;
volatile unsigned long SW3_intcount;
volatile unsigned char SW2_intflag;
volatile unsigned char SW3_intflag;
volatile unsigned long timerCount;
static volatile unsigned long g_ulBase;


//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************

// an example of how you can use structs to organize your pin settings for easier maintenance
typedef struct PinSetting {
    unsigned long port;
    unsigned int pin;
} PinSetting;

static PinSetting switch2 = { .port = GPIOA2_BASE, .pin = 0x40};

//*****************************************************************************
//                      LOCAL FUNCTION PROTOTYPES                           
//*****************************************************************************
static void BoardInit(void);

//*****************************************************************************
//                      LOCAL FUNCTION DEFINITIONS                         
//*****************************************************************************
static void TimerIntHandler(){
    Timer_IF_InterruptClear(g_ulBase);
    timerCount++;
}

static void GPIOA1IntHandler(void) { // SW3 handler
    unsigned long ulStatus;

    ulStatus = MAP_GPIOIntStatus (GPIOA3_BASE, true);
    MAP_GPIOIntClear(GPIOA3_BASE, ulStatus);        // clear interrupts on GPIOA1
    




    SW3_intflag=1;
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
static void
BoardInit(void) {
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
    
    // Enable Processor
    //
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}
//****************************************************************************
//
//! Main function
//!
//! \param none
//! 
//!
//! \return None.
//
//****************************************************************************

int decode(int num){
    int button;
    if(num == 0xA800){
        button = 0;
    }
    else if(num == 0xA880){
        button = 1;
    }
    else if(num == 0xA840){
        button = 2;
    }
    else if(num == 0xA8C0){
            button = 3;
        }
    else if(num == 0xA820){
            button = 4;
        }
    else if(num == 0xA8A0){
            button = 5;
        }
    else if(num == 0xA860){
            button = 6;
        }
    else if(num == 0xA8E0){
            button = 7;
        }
    else if(num == 0xA810){
            button = 8;
        }
    else if(num == 0xA890){
            button = 9;
        }
    else if(num == 0xA830){
            button = 10;
        }
    else if(num == 0xA854){
            button = 11;
        }
    else{
        button = -1;
    }
    return button;
}

int main() {
    unsigned long ulStatus;

    BoardInit();
    
    PinMuxConfig();
    
    InitTerm();

    ClearTerm();

    
    g_ulBase = TIMERA2_BASE;
    Message("\t\t Test \n\r");

    //
    // Register the interrupt handlers
    //
    MAP_GPIOIntRegister(GPIOA3_BASE, GPIOA1IntHandler);
    TimerIntRegister(TIMERA2_BASE,TIMER_A,TimerIntHandler);
    TimerConfigure(TIMERA2_BASE, (TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PERIODIC_UP));

    Message("\t\t Test2 \n\r");
    //
    // Configure rising edge interrupts on SW2 and SW3
    //
    MAP_GPIOIntTypeSet(GPIOA3_BASE, 0x40, GPIO_BOTH_EDGES); // SW3
    ulStatus = MAP_GPIOIntStatus(GPIOA3_BASE, false);
    MAP_GPIOIntClear(GPIOA3_BASE, ulStatus);            // clear interrupts on GPIOA1

    // clear global variables
    SW2_intcount=0;
    SW3_intcount=0;
    SW2_intflag=0;
    SW3_intflag=0;
    long wStart = 0;
    timerCount = 0;
    Message("\t\t Test3 \n\r");
    // Enable SW2 and SW3 interrupts
    MAP_GPIOIntEnable(GPIOA3_BASE, 0x40);

//    Timer_IF_Init(PRCM_TIMERA0, g_ulBase, TIMER_CFG_PERIODIC, TIMER_A, 0);
//    Timer_IF_IntSetup(g_ulBase, TIMER_A, TimerIntHandler);
    TimerLoadSet(TIMERA2_BASE, TIMER_A, 0x1F40);
    TimerIntEnable(TIMERA2_BASE,TIMER_TIMA_TIMEOUT);

    Message("\t\t****************************************************\n\r");
    Message("\t\t\tPush SW3 or SW2 to generate an interrupt\n\r");
    Message("\t\t ****************************************************\n\r");
    Message("\n\n\n\r");
    Report("Falling ints = %d\t Rising ints = %d\r\n", SW2_intcount, SW3_intcount);

    TimerEnable(TIMERA2_BASE,TIMER_A);
    int num = 0;
    int buttonPressStart = 0;
    int buttonPressEnd = 0;
    int bitcount = 0;
    int pressEndTime = 0;
    int button;
    while (1) {
        while ((SW3_intflag==0)) {;}
        if (SW3_intflag) {
            SW3_intflag=0;  // clear flag
            //falling 
            //Report("Button pressed: %d\r\n", timerCount);
                if(wStart == 0){
                    wStart = timerCount;
                    if(buttonPressEnd == 1){
                        buttonPressStart = wStart;
                    }
                }
                else{
                    //Report("%d\n\r", timerCount);
                    long wEnd = timerCount;
                    unsigned long tlength = wEnd - wStart;
                    //Report("%d\n\r\n\r", timerCount);
//                  if(tlength >= 200){
//                      //Report("Signal buttonPressEnd: %d, %d\r\n", wStart, wEnd);
//                        wStart = wEnd;
//                        pressEndTime = wEnd;
//                        buttonPressEnd = 1;
//                  }
                    if(tlength >= 80){
                        //Report("\n\r\n\r");
                        //Report("Signal start: %d, %d\r\n", wStart, wEnd);
                        wStart = wEnd;
                        bitcount = 0;
                        if(buttonPressEnd == 1){
                            pressEndTime = wEnd;
                            if(pressEndTime - buttonPressStart > 500){
                                    buttonPressEnd = 0;
                            }
                        }
                    }
                    else if(tlength >= 39){  //next signal will be bit
                        //Report("Signal intermediate: %d, %d\r\n", wStart, wEnd);
                        wStart = 0;
                    }
                    else if((tlength >= 10)){  //
                        num *= 2;
                        num += 1;
                        //Report("1\r\n");
                        wStart = 0;
                        bitcount++;
                    }
                    else if((tlength >= 4)){
                        num *= 2;
                        //Report("0\r\n");
                        wStart = 0;
                        bitcount++;
                    }
                }
                if(bitcount == 16){
                    if(buttonPressEnd == 0){
                        button = decode(num);
                        Report("%d\n\r", button);
                    }
                    buttonPressEnd = 1;
                    bitcount = 0;
                    num = 0;
                }
                if(buttonPressEnd == 1){

                }
            }
        //if((buttonPressEnd > 0) && (timerCount - buttonPressEnd < )){

        //}

    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
