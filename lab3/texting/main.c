
// Standard includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
#include "spi.h"
#include "uart.h"
#include "prcm.h"

// Common interface includes
#include "uart_if.h"
#include "timer_if.h"
#include "pin_mux_config.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1351.h"
#include "glcdfont.h"

// Color definitions
#define BLACK           0x0000
#define BLUE            0x001F
#define GREEN           0x07E0
#define CYAN            0x07FF
#define RED             0xF800
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define WHITE           0xFFFF

#define SPI_IF_BIT_RATE  100000
#define TR_BUFF_SIZE     100

extern void (* const g_pfnVectors[])(void);

volatile unsigned long SW2_intcount;
volatile unsigned long SW3_intcount;
volatile unsigned char SW2_intflag;
volatile unsigned char SW3_intflag;
volatile unsigned long timerCount;
static volatile unsigned long g_ulBase;
char* outMessage;

    //letters is a 2D matrix, where the rows correspond to the button number and the column corresponds
    // to count, or how many times a button was pressed - to account for multiple letters assigned to 
    // a button
char letters[10][5] =
        { { ' ', ' ', ' ', ' ', ' ' }, { '1', '1', '1', '1', '1' }, { 'A', 'B',
                                                                      'C', '2',
                                                                      'A' },
          { 'D', 'E', 'F', '3', 'D' }, { 'G', 'H', 'I', '4', 'G' }, { 'J', 'K',
                                                                      'L', '5',
                                                                      'J' },
          { 'M', 'N', 'O', '6', 'M' }, { 'P', 'Q', 'R', 'S', '7' }, { 'T', 'U',
                                                                      'V', '8',
                                                                      'T' },
          { 'W', 'X', 'Y', 'Z', '9' } };

static void BoardInit(void);

//*****************************************************************************
//                      LOCAL FUNCTION DEFINITIONS                         
//*****************************************************************************


//
static void TimerIntHandler()
{
    Timer_IF_InterruptClear(TIMERA2_BASE);
    timerCount++;
}


/*
    GPIO Interrupt Handler, called on rising AND falling edges
*/
static void GPIOA1IntHandler(void)
{ // SW3 handler
    unsigned long ulStatus;

    ulStatus = MAP_GPIOIntStatus(GPIOA3_BASE, true);
    MAP_GPIOIntClear(GPIOA3_BASE, ulStatus);       // clear interrupts on GPIOA1

    SW3_intflag = 1;
}

/*
    UART Interrupt Handler, called once the message string has been pushed into the UART buffer
    Reads and displays the incoming message.
*/
static void UARTIntHandler(void)
{
    UARTIntDisable(UARTA1_BASE,UART_INT_RX);
    UARTIntClear(UARTA1_BASE, UART_INT_RX);
    //Until UART buffer is empty
    while (UARTCharsAvail(UARTA1_BASE))
    {
        char sChar = UARTCharGet(UARTA1_BASE);
        printf("%c\n", sChar);
        if (sChar != '\0')
        {
            outMessage[strlen(outMessage)] = sChar;
        }

    }
    outMessage[strlen(outMessage)] = '\0';
    fillRect(0, 63, 128, 7, BLACK);
    setCursor(0, 63);
    Outstr(outMessage);
    setCursor(0, 0);
    int i = 0;
    for (i = strlen(outMessage); i > 0; i--)
    {
        outMessage[i - 1] = '\0';
    }
    UARTIntEnable(UARTA1_BASE,UART_INT_RX);
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
    MAP_IntVTableBaseSet((unsigned long) &g_pfnVectors[0]);

    // Enable Processor
    //
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}

/*
    Function to decode the bit pattern from a button press
    num: the bit pattern read in by the IR receiver
*/
int decode(int num)
{
    int button;
    if (num == 0xA800)
    {
        button = 0;
    }
    else if (num == 0xA880)
    {
        button = 1;
    }
    else if (num == 0xA840)
    {
        button = 2;
    }
    else if (num == 0xA8C0)
    {
        button = 3;
    }
    else if (num == 0xA820)
    {
        button = 4;
    }
    else if (num == 0xA8A0)
    {
        button = 5;
    }
    else if (num == 0xA860)
    {
        button = 6;
    }
    else if (num == 0xA8E0)
    {
        button = 7;
    }
    else if (num == 0xA810)
    {
        button = 8;
    }
    else if (num == 0xA890)
    {
        button = 9;
    }
    else if (num == 0xA830)
    {
        button = 10;
    }
    else if (num == 0xA854)
    {
        button = 11;
    }
    else
    {
        button = -1;
    }
    return button;
}

/*
    Translate the pressed button into a character, based on how many times the button is pressed 
    num: button number
    count: number of times the button was pressed
*/
char textTranslate(int num, int count)
{
    //To account for the fact that 7 and 9 have 5 characters assigned to them instead of 4
    if (num != 7 && num != 9)
    {
        return letters[num][count % 4];
    }
    else
    {
        return letters[num][count % 5];
    }
}

int main()
{
    // Boilerplate 
    unsigned long ulStatus;

    BoardInit();

    PinMuxConfig();

    MAP_PRCMPeripheralClkEnable(PRCM_GSPI, PRCM_RUN_MODE_CLK);

    InitTerm();

    ClearTerm();
    printf("Test\n\r");

    g_ulBase = TIMERA2_BASE;

    //Max message size is 512 characters
    outMessage = (char*) malloc(sizeof(char) * 512);
    int i = 0;
    for(i = 0; i < 512; i++){
        outMessage[i] = '\0';
    }

    //Set Up UART
    UARTIntRegister(UARTA1_BASE, UARTIntHandler);
    UARTIntClear(UARTA1_BASE, UART_INT_RX);
    UARTIntEnable(UARTA1_BASE, UART_INT_RX);
    UARTFIFOEnable(UARTA1_BASE);
    UARTFIFOLevelSet(UARTA1_BASE,UART_FIFO_TX1_8,UART_FIFO_RX1_8);
    UARTConfigSetExpClk(UARTA1_BASE, PRCMPeripheralClockGet(PRCM_UARTA1), 115200,
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                        UART_CONFIG_PAR_NONE));
    
    //
    // Register the interrupt handler
    //
    MAP_GPIOIntRegister(GPIOA3_BASE, GPIOA1IntHandler);
    TimerIntRegister(TIMERA2_BASE, TIMER_A, TimerIntHandler);
    
    TimerConfigure(TIMERA2_BASE,
                   (TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PERIODIC_UP));

    //
    // Configure both edge interrupts on IR Receiver pin
    //
    MAP_GPIOIntTypeSet(GPIOA3_BASE, 0x40, GPIO_BOTH_EDGES);
    ulStatus = MAP_GPIOIntStatus(GPIOA3_BASE, false);
    MAP_GPIOIntClear(GPIOA3_BASE, ulStatus);       // clear interrupts on GPIOA3

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
    SPI_IF_BIT_RATE,
                           SPI_MODE_MASTER, SPI_SUB_MODE_0, (SPI_SW_CTRL_CS |
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

    // Enable GPIO interrupt
    MAP_GPIOIntEnable(GPIOA3_BASE, 0x40);

    TimerLoadSet(TIMERA2_BASE, TIMER_A, 0x1F40);
    TimerIntEnable(TIMERA2_BASE, TIMER_TIMA_TIMEOUT);

    //Enable the Timer
    TimerEnable(TIMERA2_BASE, TIMER_A);

    int num = 0;
    int buttonPressStart = 0;
    int buttonPressEnd = 0;
    int bitcount = 0;
    int pressEndTime = 0;
    int button = 20;
    int prevButton = 13;
    int sep = 0;
    char* message = (char*) malloc(sizeof(char) * 512);

    //Clearing the malloc'd memory to ensure no garbage values in string
    for (i = 0; i < 512; i++)
    {
        message[i] = '\0';
    }

    int msgLength = 0;
    int buttonIdx = 0;
    fillScreen(BLACK);
    setTextColor(WHITE, BLACK);

    while (1)
    {   
        //Waiting until GPIO interrupt hits
        while ((SW3_intflag == 0))
        {
            ;
        }
        if (SW3_intflag)
        {
            SW3_intflag = 0;  // clear flag
            //falling 
            //Report("Button pressed: %d\r\n", timerCount);

            //starting edge of the pulse 
            if (wStart == 0)
            {
                wStart = timerCount;
                if (buttonPressEnd == 1)
                {
                    buttonPressStart = wStart; // record the pulse start time
                }
            }
            else
            {
                long wEnd = timerCount;
                unsigned long tlength = wEnd - wStart;

                //detecting long preamble pulse
                if (tlength >= 80)
                {
                    wStart = wEnd; //don't reset pulse because preamble pulse has 3 edges
                    bitcount = 0;   //reset the bit count in case of a new button press
                    
                    //Check for signal repetition
                    if (buttonPressEnd == 1)
                    {
                        pressEndTime = wEnd;  //record timing of the end of the button press
                        //Enough time between prev signal and new one for new one not to be a repeat
                        if (pressEndTime - buttonPressStart > 500)
                        {
                            buttonPressEnd = 0;
                            prevButton = button; // save the previously detected button
                            sep = 0;
                        }
                        //Enough time between prev signal and new one to record fresh character
                        if (pressEndTime - buttonPressStart > 10000)
                        {
                            sep = 1;
                        }
                    }
                }
                //second half of preamble pulse
                else if (tlength >= 39)
                {  //next signal will be bit
                    wStart = 0;
                }
                //Detect bit value 1 
                else if ((tlength >= 10))
                {  //
                    num *= 2;  //shift current num value right by 1
                    num += 1;   //set last bit to 1
                    wStart = 0;
                    bitcount++;
                }
                //Detect bit value 0
                else if ((tlength >= 4))
                {
                    num *= 2;
                    wStart = 0;
                    bitcount++;
                }
            }
            if (bitcount == 16)
            {
                //The signal is not a repetition
                if (buttonPressEnd == 0)
                {
                    button = decode(num);
                    //Repeated button press
                    if (prevButton == button && button != -1)
                    {
                        if (sep == 0)
                        {
                            //switch letter
                            if (button >= 0 && button < 10)
                            {

                                int len = strlen(message);
                                message[len - 1] = textTranslate(button,
                                                                 buttonIdx);
                                buttonIdx++;
                            }
                            else
                            {
                                //Deleting characters
                                if (button == 10 && (strlen(message)) > 0)
                                {
                                    int len = strlen(message);
                                    message[len - 1] = '\0';
                                    msgLength =
                                            msgLength > 0 ? (msgLength - 1) : 0;
                                    fillRect(0, 0, 128, 7, BLACK);
                                    setCursor(0, 0);
                                    Outstr(message);
                                    setCursor(0, 0);
                                }
                                //Enter key press, sending message to UART
                                else if (button == 11 && (strlen(message)) > 0)
                                {
                                    int i = 0;
                                    for (; i < strlen(message); i++)
                                    {
                                        UARTCharPut(UARTA1_BASE ,message[i]);
                                    }
                                    //UART Interrupt triggered here
                                }
                            }
                        }
                        else
                        {
                            //Add character to end of string
                            if (button >= 0 && button < 10)
                            {
                                int len = strlen(message);
                                msgLength++;
                                buttonIdx = 0;
                                message[len] = textTranslate(button, buttonIdx);

                            }
                            else
                            {
                                if (button == 10 && (strlen(message)) > 0)
                                {
                                    int len = strlen(message);
                                    message[len - 1] = '\0';
                                    msgLength =
                                            msgLength > 0 ? msgLength - 1 : 0;
                                    fillRect(0, 0, 128, 7, BLACK);
                                    setCursor(0, 0);
                                    Outstr(message);
                                    setCursor(0, 0);
                                }
                                else if (button == 11 && (strlen(message)) > 0)
                                {
                                    int i = 0;
                                    for (; i < strlen(message); i++)
                                    {
                                        UARTCharPut(UARTA1_BASE ,message[i]);
                                    }

                                }
                            }
                        }

                    }
                    //Different button pressed
                    else if (prevButton != button && button != -1)
                    {
                        buttonIdx = 0;
                        if (button >= 0 && button < 10)
                        {
                            int len = strlen(message);
                            message[len] = textTranslate(button, buttonIdx);
                            buttonIdx++;
                            msgLength++;

                        }
                        else
                        {
                            if (button == 10 && (strlen(message)) > 0)
                            {
                                int len = strlen(message);
                                message[len - 1] = '\0';
                                msgLength = msgLength > 0 ? msgLength - 1 : 0;
                                fillRect(0, 0, 128, 7, BLACK);
                                setCursor(0, 0);
                                Outstr(message);
                                setCursor(0, 0);
                            }
                            else if (button == 11 && (strlen(message)) > 0)
                            {
                                int i = 0;
                                for (; i < strlen(message); i++)
                                {
                                    UARTCharPut(UARTA1_BASE ,message[i]);
                                }

                            }
                        }
                    }
                    int len = strlen(message);
                    printf("%s: %d, b: %d, pb: %d\n\r", message, len, button,
                           prevButton);
                    Outstr(message);
                    setCursor(0, 0);
                }
                buttonPressEnd = 1;
                bitcount = 0;
                num = 0;
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
