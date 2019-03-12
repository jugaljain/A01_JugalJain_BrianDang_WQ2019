/*
 * Copyright (c) 2016, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,

 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdint.h>
#include <ti/sysbios/posix/pthread.h>
#include <ti/sysbios/BIOS.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/net/wifi/simplelink.h>

#include "Board.h"
#include "uart_term.h"

#define TEMBOO_THREAD_STACK_SIZE 8192
#define SL_TASK_STACK_SIZE 2048
#define SL_TASK_PRIORITY 9

extern void* mainThread(void* args);

int main(void) {
    pthread_t choreoThread = NULL;
    pthread_t slTaskThread = NULL;
    pthread_attr_t choreoThreadAttrs;
    pthread_attr_t slTaskThreadAttrs;
    struct sched_param choreoThreadParam;
    struct sched_param slTaskThreadParam;

    // Call board init functions
    // Comment out any unused drivers
    Board_initGeneral();
    Board_initGPIO();
    Board_initSPI();
    Board_initPWM();
    Board_initADC();

    // Begin UART
    UART_Handle uartHandle = InitTerm();
    UART_control(uartHandle, UART_CMD_RXDISABLE, NULL);

    // Initialize thread attributes and set priority
    pthread_attr_init(&choreoThreadAttrs);
    pthread_attr_init(&slTaskThreadAttrs);
    choreoThreadParam.sched_priority = 1;
    slTaskThreadParam.sched_priority = SL_TASK_PRIORITY;

    // Set Choreo thread attributes and create thread
    int rc = pthread_attr_setdetachstate(&choreoThreadAttrs, PTHREAD_CREATE_DETACHED);
    rc |= pthread_attr_setschedparam(&choreoThreadAttrs, &choreoThreadParam);
    rc |= pthread_attr_setstacksize(&choreoThreadAttrs, TEMBOO_THREAD_STACK_SIZE);
    rc |= pthread_create(&choreoThread, &choreoThreadAttrs, mainThread, NULL);
    if (rc != 0) {
        UART_PRINT("Could not create choreoThread\n\r");
        while (1);
    }

    // Set sl_task Thread attributes and create thread
    rc = pthread_attr_setschedparam(&slTaskThreadAttrs, &slTaskThreadParam);
    rc |= pthread_attr_setstacksize(&slTaskThreadAttrs, SL_TASK_STACK_SIZE);
    rc |= pthread_attr_setdetachstate(&slTaskThreadAttrs, PTHREAD_CREATE_DETACHED);
    rc |= pthread_create(&slTaskThread, &slTaskThreadAttrs, sl_Task, NULL);
    if (rc != 0) {
        UART_PRINT("Could not create simplelink task\n\r");
        while (1);
    }

    BIOS_start();

    return 0;
}
