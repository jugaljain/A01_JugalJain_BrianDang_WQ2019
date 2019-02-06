
//*****************************************************************************
//
// Application Name     - int_sw
// Application Overview - The objective of this application is to demonstrate
//							GPIO interrupts using SW2 and SW3.
//							NOTE: the switches are not debounced!
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
volatile unsigned char timeCount;
volatile unsigned char prevTime;


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
    MAP_GPIOIntClear(GPIOA3_BASE, ulStatus);		// clear interrupts on GPIOA1
    


    if(prevTime == 0){
        prevTime = timerCount;
    }else{
        unsigned char tlength = timerCount - prevTime;
        if(tlength >= 80){
            prevTime = timerCount;
            Report("Signal length: %d", tlength);
        }
        else if(tlength >= 39){  //next signal will be bit
            prevTime = 0;
            Report("Signal length: %d", tlength);
        }
        else if(tlength >= 14){  //
            prevTime = 0;
            Report("Signal length: %d", tlength);
        }
    }

    SW3_intflag=1;
}



static void GPIOA2IntHandler(void) {	// SW2 handler
	unsigned long ulStatus;

    ulStatus = MAP_GPIOIntStatus (GPIOA3_BASE, true);
    MAP_GPIOIntClear(GPIOA3_BASE, ulStatus);		// clear interrupts on GPIOA2
    SW2_intcount++;
    SW2_intflag=1;
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
int main() {
	unsigned long ulStatus;

    BoardInit();
    
    PinMuxConfig();
    
    InitTerm();

    ClearTerm();

    



    //
    // Register the interrupt handlers
    //
    MAP_GPIOIntRegister(GPIOA3_BASE, GPIOA1IntHandler);

    //
    // Configure rising edge interrupts on SW2 and SW3
    //
    MAP_GPIOIntTypeSet(GPIOA3_BASE, 0x40, GPIO_BOTH_EDGES);	// SW3
    ulStatus = MAP_GPIOIntStatus(GPIOA3_BASE, false);
    MAP_GPIOIntClear(GPIOA3_BASE, ulStatus);			// clear interrupts on GPIOA1

    g_ulBase = TIMERA0_BASE;
    // clear global variables
    SW2_intcount=0;
    SW3_intcount=0;
    SW2_intflag=0;
    SW3_intflag=0;
    prevTime = 0;
    timerCount = 0;

    // Enable SW2 and SW3 interrupts
    MAP_GPIOIntEnable(GPIOA3_BASE, 0x40);

    Timer_IF_Init(PRCM_TIMERA0, g_ulBase, TIMER_CFG_PERIODIC, TIMER_A, 0);
    Timer_IF_IntSetup(g_ulBase, TIMER_A, TimerBaseIntHandler);
    TimerLoadSet(g_ulBase, TIMER_A, 8000);

    Message("\t\t****************************************************\n\r");
    Message("\t\t\tPush SW3 or SW2 to generate an interrupt\n\r");
    Message("\t\t ****************************************************\n\r");
    Message("\n\n\n\r");
	Report("Falling ints = %d\t Rising ints = %d\r\n", SW2_intcount, SW3_intcount);

    while (1) {
    	while ((SW2_intflag==0) && (SW3_intflag==0)) {;}
    	if (SW3_intflag) {
    		SW3_intflag=0;	// clear flag
            //falling 
    		
    	}

    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
