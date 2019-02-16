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
char* outMessage;
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

//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************

// an example of how you can use structs to organize your pin settings for easier maintenance
typedef struct PinSetting
{
    unsigned long port;
    unsigned int pin;
} PinSetting;

static PinSetting switch2 = { .port = GPIOA2_BASE, .pin = 0x40 };

//*****************************************************************************
//                      LOCAL FUNCTION PROTOTYPES                           
//*****************************************************************************
static void BoardInit(void);

//*****************************************************************************
//                      LOCAL FUNCTION DEFINITIONS                         
//*****************************************************************************
static void TimerIntHandler()
{
    Timer_IF_InterruptClear(TIMERA2_BASE);
    timerCount++;
}

static void GPIOA1IntHandler(void)
{ // SW3 handler
    unsigned long ulStatus;

    ulStatus = MAP_GPIOIntStatus(GPIOA3_BASE, true);
    MAP_GPIOIntClear(GPIOA3_BASE, ulStatus);       // clear interrupts on GPIOA1

    SW3_intflag = 1;
}

static void UARTIntHandler(void)
{
    printf("Inside handler\n\r");
    UARTIntDisable(UARTA1_BASE,UART_INT_RX);
    UARTIntClear(UARTA1_BASE, UART_INT_RX);
    printf("before loop\n");
    while (UARTCharsAvail(UARTA1_BASE))
    {
        printf("inside loop\n");
        char sChar = UARTCharGet(UARTA1_BASE);
        printf("%c\n", sChar);
        if (sChar != '\0')
        {
            printf("%d\n", strlen(outMessage));
            outMessage[strlen(outMessage)] = sChar;
            printf("%s:not null character\n", outMessage);
        }

    }
    outMessage[strlen(outMessage)] = '\0';
    fillRect(0, 63, 128, 7, BLACK);
    setCursor(0, 63);
    Outstr(outMessage);
    setCursor(0, 0);
    printf("%s \n", outMessage);
    int i = 0;
    for (i = strlen(outMessage); i > 0; i--)
    {
        outMessage[i - 1] = '\0';
    }
    printf("%d\n", strlen(outMessage));
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

char textTranslate(int num, int count)
{
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
    unsigned long ulStatus;

    BoardInit();

    PinMuxConfig();

    MAP_PRCMPeripheralClkEnable(PRCM_GSPI, PRCM_RUN_MODE_CLK);

    InitTerm();

    ClearTerm();
    printf("Test\n\r");

    g_ulBase = TIMERA2_BASE;

    outMessage = (char*) malloc(sizeof(char) * 512);
    int i = 0;
    for(i = 0; i < 512; i++){
        outMessage[i] = '\0';
    }
    //Report("Test1\n\r");
    UARTIntRegister(UARTA1_BASE, UARTIntHandler);
    UARTIntClear(UARTA1_BASE, UART_INT_RX);
    UARTIntEnable(UARTA1_BASE, UART_INT_RX);
    UARTFIFOEnable(UARTA1_BASE);
    UARTFIFOLevelSet(UARTA1_BASE,UART_FIFO_TX1_8,UART_FIFO_RX1_8);
    UARTConfigSetExpClk(UARTA1_BASE, PRCMPeripheralClockGet(PRCM_UARTA1), 115200,
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                        UART_CONFIG_PAR_NONE));
    //UARTEnable(UARTA1_BASE);
    //Report("Test2\n\r");
    //
    // Register the interrupt handlers
    //
    MAP_GPIOIntRegister(GPIOA3_BASE, GPIOA1IntHandler);
    TimerIntRegister(TIMERA2_BASE, TIMER_A, TimerIntHandler);
    TimerConfigure(TIMERA2_BASE,
                   (TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PERIODIC_UP));

    //
    // Configure rising edge interrupts on SW2 and SW3
    //
    MAP_GPIOIntTypeSet(GPIOA3_BASE, 0x40, GPIO_BOTH_EDGES); // SW3
    ulStatus = MAP_GPIOIntStatus(GPIOA3_BASE, false);
    MAP_GPIOIntClear(GPIOA3_BASE, ulStatus);       // clear interrupts on GPIOA1

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

    // Enable SW2 and SW3 interrupts
    MAP_GPIOIntEnable(GPIOA3_BASE, 0x40);

//    Timer_IF_Init(PRCM_TIMERA0, g_ulBase, TIMER_CFG_PERIODIC, TIMER_A, 0);
//    Timer_IF_IntSetup(g_ulBase, TIMER_A, TimerIntHandler);
    TimerLoadSet(TIMERA2_BASE, TIMER_A, 0x1F40);
    TimerIntEnable(TIMERA2_BASE, TIMER_TIMA_TIMEOUT);

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
        while ((SW3_intflag == 0))
        {
            ;
        }
        if (SW3_intflag)
        {
            SW3_intflag = 0;  // clear flag
            //falling 
            //Report("Button pressed: %d\r\n", timerCount);
            if (wStart == 0)
            {
                wStart = timerCount;
                if (buttonPressEnd == 1)
                {
                    buttonPressStart = wStart;
                }
            }
            else
            {
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
                if (tlength >= 80)
                {
                    //Report("\n\r\n\r");
                    //Report("Signal start: %d, %d\r\n", wStart, wEnd);
                    wStart = wEnd;
                    bitcount = 0;
                    if (buttonPressEnd == 1)
                    {
                        pressEndTime = wEnd;
                        if (pressEndTime - buttonPressStart > 500)
                        {
                            buttonPressEnd = 0;
                            prevButton = button;
                            sep = 0;
                        }
                        if (pressEndTime - buttonPressStart > 10000)
                        {
                            sep = 1;
                        }
                    }
                }
                else if (tlength >= 39)
                {  //next signal will be bit
                   //Report("Signal intermediate: %d, %d\r\n", wStart, wEnd);
                    wStart = 0;
                }
                else if ((tlength >= 10))
                {  //
                    num *= 2;
                    num += 1;
                    //Report("1\r\n");
                    wStart = 0;
                    bitcount++;
                }
                else if ((tlength >= 4))
                {
                    num *= 2;
                    //Report("0\r\n");
                    wStart = 0;
                    bitcount++;
                }
            }
            if (bitcount == 16)
            {
                //Report("Test3\n\r");
                if (buttonPressEnd == 0)
                {
                    button = decode(num);
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
                        else
                        {
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
            //if((buttonPressEnd > 0) && (timerCount - buttonPressEnd < )){

            //}

        }
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
