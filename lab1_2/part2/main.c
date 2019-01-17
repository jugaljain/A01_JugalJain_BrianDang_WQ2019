//*****************************************************************************
//
// Students               Brian Dang, Jugal Jain
// Application Name     - Lab1 Part 2
// Application Overview - The objective of this application is to showcase the 
//                        GPIO control using Driverlib api calls. The LEDs 
//                        connected to the GPIOs on the LP are used to indicate 
//                        the GPIO output. The GPIOs are driven high-low 
//                        periodically in order to turn on-off the LEDs.
// Application Details  -
// http://processors.wiki.ti.com/index.php/CC32xx_Blinky_Application
// or
// docs\examples\CC32xx_Blinky_Application.pdf
//
//*****************************************************************************

//****************************************************************************
//
//! \addtogroup blinky
//! @{
//
//****************************************************************************

// Standard includes
#include <stdio.h>
#include <stdlib.h>

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
//#include "uart.h"
#include "uart_if.h"



// Common interface includes
#include "gpio_if.h"

#include "pin_mux_config.h"

#define APPLICATION_VERSION     "1.1.1"

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
//                      LOCAL FUNCTION PROTOTYPES                           
//*****************************************************************************
void LEDBlinkyRoutine();
static void BoardInit(void);

//*****************************************************************************
//                      LOCAL FUNCTION DEFINITIONS                         
//*****************************************************************************

//*****************************************************************************
//
//! Configures the pins as GPIOs and peroidically toggles the lines
//!
//! \param None
//! 
//! This function  
//!    1. Configures 3 lines connected to LEDs as GPIO
//!    2. Sets up the GPIO pins as output
//!    3. Periodically toggles each LED one by one by toggling the GPIO line
//!
//! \return None
//
//*****************************************************************************

//blinks LEDs on and off if switch 2 is pushed
void sw2pushed(){
    GPIO_IF_LedOff(MCU_ALL_LED_IND);
    while (1){
        //LED blinking
        MAP_UtilsDelay(8000000);
        GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        GPIO_IF_LedOn(MCU_ORANGE_LED_GPIO);
        GPIO_IF_LedOn(MCU_GREEN_LED_GPIO);
        MAP_UtilsDelay(8000000);
        GPIO_IF_LedOff(MCU_RED_LED_GPIO);
        GPIO_IF_LedOff(MCU_ORANGE_LED_GPIO);
        GPIO_IF_LedOff(MCU_GREEN_LED_GPIO);
        int sw3 = GPIOPinRead(GPIOA1_BASE, 0x20);

        //checks if switch 3 is pushed
        if(sw3 == 0x20){
             Message("SW3 pushed\n\r");
             GPIOPinWrite(GPIOA3_BASE, 0x10, 0x00);
             sw3pushed();
        }

    }
}

//displays binary if switch 3 is pushed
void sw3pushed(){

    GPIO_IF_LedOff(MCU_ALL_LED_IND);
    while(1){
        int i;
        //for loop calculates which led is on for what number
        for(i = 0; i < 8; i++){
            if(i % 2 != 0){
                GPIO_IF_LedOn(MCU_RED_LED_GPIO);
            }
            if(i % 4 == 2 || i % 4 == 3){
                GPIO_IF_LedOn(MCU_ORANGE_LED_GPIO);
            }
            if(i % 8 == 4 || i % 8 == 5|| i % 8 == 6|| i % 8 == 7){
                GPIO_IF_LedOn(MCU_GREEN_LED_GPIO);
            }

            //turns LEDs off before displaying next binary number
            MAP_UtilsDelay(8000000);
            GPIO_IF_LedOff(MCU_RED_LED_GPIO);
            GPIO_IF_LedOff(MCU_ORANGE_LED_GPIO);
            GPIO_IF_LedOff(MCU_GREEN_LED_GPIO);
            MAP_UtilsDelay(8000000);
            int sw2 = GPIOPinRead(GPIOA2_BASE, 0x40);

            if(sw2 == 0x40){  //check if pin_15, the pin for sw2 is on or off
                Message("SW2 pushed\n\r");
                GPIOPinWrite(GPIOA3_BASE, 0x10, 0xFF);
                sw2pushed();
            }
        }



    }
}

void start()
{
    //
    // Method polls for switch pushes
    while(1)
    {
        //initializing switch pins
        int sw3 = GPIOPinRead(GPIOA1_BASE, 0x20);
        int sw2 = GPIOPinRead(GPIOA2_BASE, 0x40);

        //polls for switch 3
        if(sw3 == 0x20){
            Message("SW3 pushed\n\r");
            GPIOPinWrite(GPIOA3_BASE, 0x10, 0x00);
            sw3pushed();
        } 

        if(sw2 == 0x40){  //check if pin_15, the pin for sw2 is on or off
            Message("SW2 pushed\n\r");
            GPIOPinWrite(GPIOA3_BASE, 0x10, 0xFF);
            sw2pushed();
        }
    }

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
//****************************************************************************
//
//! Main function
//!
//! \param none
//! 
//! This function  
//!    1. Invokes the LEDBlinkyTask
//!
//! \return None.
//
//****************************************************************************
int
main()
{
    //
    // Initialize Board configurations
    //
    BoardInit();
        //
        // Muxing for Enabling UART_TX and UART_RX.
        //
        PinMuxConfig();
        //
        // Initialising the Terminal.
        //
        InitTerm();
        //
        // Clearing the Terminal.
        //
        ClearTerm();
        //displaying instructions to terminal
        Message("\t\t****************************************************\n\r");
        Message("\t\t\t        CC3200 GPIO Application        \n\r");
        Message("\t\t****************************************************\n\n\n\n\n\n\r");
        Message("\t\t****************************************************\n\r");
        Message("\t\t Push SW3 to start LED binary counting  \n\r");
        Message("\t\t Push SW2 to blink LEDs on and off \n\r") ;

        Message("\t\t ****************************************************\n\r");
        Message("\n\n\n\r");
    //
    // Power on the corresponding GPIO port B for 9,10,11.
    // Set up the GPIO lines to mode 0 (GPIO)
    //
    GPIO_IF_LedConfigure(LED1|LED2|LED3);

    GPIO_IF_LedOff(MCU_ALL_LED_IND);

    //
    // Start the LEDBlinkyRoutine
    //
    start();
    return 0;
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
