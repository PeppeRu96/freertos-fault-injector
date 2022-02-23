/*
 * FreeRTOS V202111.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

 /* Standard includes. */
#include "stdio.h"
#include "string.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "message_buffer.h"

/* Demo app includes. */
#include "MessageBufferDemo.h"

/* The number of bytes of storage in the message buffers used in this test. */
#define mbMESSAGE_BUFFER_LENGTH_BYTES      ( ( size_t ) 50 )

/* The number of additional bytes used to store the length of each message. */
#define mbBYTES_TO_STORE_MESSAGE_LENGTH    ( sizeof( configMESSAGE_BUFFER_LENGTH_TYPE ) )

/* Start and end ASCII characters used in messages sent to the buffers. */
#define mbASCII_SPACE                      32
#define mbASCII_TILDA                      126

/* Defines the number of tasks to create in this test and demo. */
#define mbNUMBER_OF_ECHO_CLIENTS           ( 2 )

/* Priority of the test tasks.  The send and receive go from low to high
 * priority tasks, and from high to low priority tasks. */
#define mbBASE_PRIORITY                    ( tskIDLE_PRIORITY )
#define mbLOWER_PRIORITY1                  ( mbBASE_PRIORITY )
#define mbHIGHER_PRIORITY1                 ( mbBASE_PRIORITY + 1 )
#define mbLOWER_PRIORITY2                  ( mbBASE_PRIORITY + 2 )
#define mbHIGHER_PRIORITY2                 ( mbBASE_PRIORITY + 3 )

 /* Block times used when sending and receiving from the message buffers. */
#define mbRX_TX_BLOCK_TIME                 pdMS_TO_TICKS( 175UL )

/*-----------------------------------------------------------*/

/*
 * Tests sending and receiving various lengths of messages via a message buffer.
 * The echo client sends the messages to the echo server, which then sends the
 * message back to the echo client which, checks it receives exactly what it
 * sent.
 */
static void prvEchoClient(void* pvParameters);
static void prvEchoServer(void* pvParameters);

/*-----------------------------------------------------------*/

/* The buffers used by the echo client and server tasks. */
typedef struct ECHO_MESSAGE_BUFFERS
{
    /* Handles to the data structures that describe the message buffers. */
    MessageBufferHandle_t xEchoClientBuffer;
    MessageBufferHandle_t xEchoServerBuffer;
} EchoMessageBuffers_t;
static uint32_t ulEchoLoopCounters[mbNUMBER_OF_ECHO_CLIENTS] = { 0 };

/* A message that is longer than the buffer, parts of which are written to the
 * message buffer to test writing different lengths at different offsets. */
static const char* pc55ByteString = "One two three four five six seven eight nine ten eleve";

/* Remember the required stack size so tasks can be created at run time (after
 * initialisation time. */
static configSTACK_DEPTH_TYPE xBlockingStackSize = 0;

static TaskHandle_t xTaskEchoServer1, xTaskEchoServer2, xTaskEchoClient1, xTaskEchoClient2;

static volatile BaseType_t xTasksAlive = 4;

typedef struct ECHO_PARAMETERS
{
    EchoMessageBuffers_t echoMessageBuffers;
    uint32_t echoClientNumber;
} EchoParameters_t;

/*-----------------------------------------------------------*/

void vStartMessageBufferTasks(configSTACK_DEPTH_TYPE xStackSize)
{
    MessageBufferHandle_t xMessageBuffer;

#ifndef configMESSAGE_BUFFER_BLOCK_TASK_STACK_SIZE
    xBlockingStackSize = (xStackSize + (xStackSize >> 1U));
#else
    xBlockingStackSize = configMESSAGE_BUFFER_BLOCK_TASK_STACK_SIZE;
#endif

    uint32_t echoNumber;

    /* The echo servers sets up the message buffers before creating the echo
     * client tasks.  One set of tasks has the server as the higher priority, and
     * the other has the client as the higher priority. */

    echoNumber = 1;
    xTaskCreate(prvEchoServer, "1EchoServer", xBlockingStackSize, (void*)echoNumber, mbHIGHER_PRIORITY1, &xTaskEchoServer1);

    echoNumber = 2;
    xTaskCreate(prvEchoServer, "2EchoServer", xBlockingStackSize, (void*)echoNumber, mbLOWER_PRIORITY2, &xTaskEchoServer2);

    /* log the task handles */
    log_struct("MessageBuffer_TaskEchoServer1", TYPE_TASK_HANDLE, xTaskEchoServer1);
    log_struct("MessageBuffer_TaskEchoServer2", TYPE_TASK_HANDLE, xTaskEchoServer2);
}
/*-----------------------------------------------------------*/


static void prvEchoClient(void* pvParameters)
{
    size_t xSendLength = 0, ux;
    char* pcStringToSend, * pcStringReceived, cNextChar = mbASCII_SPACE;
    const TickType_t xTicksToWait = pdMS_TO_TICKS(50);


    /* The task's priority is used as an index into the loop counters used to
     * indicate this task is still running. */
    UBaseType_t uxIndex = uxTaskPriorityGet(NULL);

    /* Pointers to the client and server message buffers are passed into this task
     * using the task's parameter. */
    EchoParameters_t* parameters = (EchoParameters_t*)pvParameters;

    EchoMessageBuffers_t pxMessageBuffers = parameters->echoMessageBuffers;
    uint32_t clientNumber = parameters->echoClientNumber;

    /* Prevent compiler warnings. */
    (void)pvParameters;

    /* Create the buffer into which strings to send to the server will be
     * created, and the buffer into which strings echoed back from the server will
     * be copied. */
    pcStringToSend = (char*)pvPortMalloc(mbMESSAGE_BUFFER_LENGTH_BYTES);
    pcStringReceived = (char*)pvPortMalloc(mbMESSAGE_BUFFER_LENGTH_BYTES);

    configASSERT(pcStringToSend);
    configASSERT(pcStringReceived);

    for (; ; )
    {
        /* Generate the length of the next string to send. */
        xSendLength++;


        /* The message buffer is being used to hold variable length data, so
         * each data item requires sizeof( size_t ) bytes to hold the data's
         * length, hence the sizeof() in the if() condition below. */
        if (xSendLength > (mbMESSAGE_BUFFER_LENGTH_BYTES - sizeof(size_t)))
        {
            console_print("MessageBuffer - Echo Client %d terminated.\n", clientNumber);
            xTasksAlive--;
            vTaskDelete(NULL);
        }


        console_print("MessageBuffer - Echo Client %d preparing to send a string of size %d\n", clientNumber, xSendLength);
        memset(pcStringToSend, 0x00, mbMESSAGE_BUFFER_LENGTH_BYTES);

        for (ux = 0; ux < xSendLength; ux++)
        {
            pcStringToSend[ux] = cNextChar;

            cNextChar++;

            if (cNextChar > mbASCII_TILDA)
            {
                cNextChar = mbASCII_SPACE;
            }
        }

        /* Send the generated string to the buffer. */
        do
        {
            ux = xMessageBufferSend(pxMessageBuffers.xEchoClientBuffer, (void*)pcStringToSend, xSendLength, xTicksToWait);

            if (ux == 0)
            {
                mtCOVERAGE_TEST_MARKER();
            }
        } while (ux == 0);

        console_print("MessageBuffer - Echo Client %d sent message \"%s\"\n", clientNumber, pcStringToSend);

        /* Wait for the string to be echoed back. */
        memset(pcStringReceived, 0x00, mbMESSAGE_BUFFER_LENGTH_BYTES);
        xMessageBufferReceive(pxMessageBuffers.xEchoServerBuffer, (void*)pcStringReceived, xSendLength, portMAX_DELAY);

        console_print("MessageBuffer - Echo Client %d received back message \"%s\"\n", clientNumber, pcStringReceived);

        configASSERT(strcmp(pcStringToSend, pcStringReceived) == 0);
    }
}
/*-----------------------------------------------------------*/

static void prvEchoServer(void* pvParameters)
{
    char log_struct_name[200];
    MessageBufferHandle_t xTempMessageBuffer;
    size_t xReceivedLength;
    char* pcReceivedString;
    EchoMessageBuffers_t xMessageBuffers;
    TickType_t xTimeOnEntering;
    const TickType_t xTicksToBlock = pdMS_TO_TICKS(250UL);

    EchoParameters_t clientParameters;

    uint32_t echoNumber = (uint32_t)pvParameters;

    /* Create the message buffer used to send data from the client to the server,
     * and the message buffer used to echo the data from the server back to the
     * client. */
    xMessageBuffers.xEchoClientBuffer = xMessageBufferCreate(mbMESSAGE_BUFFER_LENGTH_BYTES);
    xMessageBuffers.xEchoServerBuffer = xMessageBufferCreate(mbMESSAGE_BUFFER_LENGTH_BYTES);
    configASSERT(xMessageBuffers.xEchoClientBuffer);
    configASSERT(xMessageBuffers.xEchoServerBuffer);

    // Log data structures
    sprintf(log_struct_name, "MessageBuffer_EchoClientBuffer_%d", echoNumber);
    log_struct(log_struct_name, TYPE_MESSAGE_BUFFER_HANDLE, xMessageBuffers.xEchoClientBuffer);
    sprintf(log_struct_name, "MessageBuffer_EchoServerBuffer_%d", echoNumber);
    log_struct(log_struct_name, TYPE_MESSAGE_BUFFER_HANDLE, xMessageBuffers.xEchoServerBuffer);

    /* Create the buffer into which received strings will be copied. */
    pcReceivedString = (char*)pvPortMalloc(mbMESSAGE_BUFFER_LENGTH_BYTES);
    configASSERT(pcReceivedString);

    /* Don't expect to receive anything yet! */
    xTimeOnEntering = xTaskGetTickCount();
    xReceivedLength = xMessageBufferReceive(xMessageBuffers.xEchoClientBuffer, (void*)pcReceivedString, mbMESSAGE_BUFFER_LENGTH_BYTES, xTicksToBlock);
    configASSERT(((TickType_t)(xTaskGetTickCount() - xTimeOnEntering)) >= xTicksToBlock);
    configASSERT(xReceivedLength == 0);
    (void)xTimeOnEntering; /* In case configASSERT() is not defined. */


    clientParameters.echoMessageBuffers = xMessageBuffers;
    clientParameters.echoClientNumber = echoNumber;


    /* Now the message buffers have been created the echo client task can be
     * created.  If this server task has the higher priority then the client task
     * is created at the lower priority - if this server task has the lower
     * priority then the client task is created at the higher priority. */
    if (echoNumber == 1)
    {
        xTaskCreate(prvEchoClient, "EchoClient", configMINIMAL_STACK_SIZE, (void*)&clientParameters, mbLOWER_PRIORITY1, &xTaskEchoClient1);
        sprintf(log_struct_name, "MessageBuffer_EchoClientTCB_%d", echoNumber);
        log_struct(log_struct_name, TYPE_TASK_HANDLE, xTaskEchoClient1);
    }
    else
    {
        xTaskCreate(prvEchoClient, "EchoClient", configMINIMAL_STACK_SIZE, (void*)&clientParameters, mbHIGHER_PRIORITY2, &xTaskEchoClient2);
        sprintf(log_struct_name, "MessageBuffer_EchoClientTCB_%d", echoNumber);
        log_struct(log_struct_name, TYPE_TASK_HANDLE, xTaskEchoClient2);
    }

    for (; ; )
    {
        memset(pcReceivedString, 0x00, mbMESSAGE_BUFFER_LENGTH_BYTES);

        if (echoNumber == 1) {
            if (eTaskGetState(xTaskEchoClient1) == eDeleted)
            {
                console_print("MessageBuffer - Echo server %d terminated.\n", echoNumber);
                xTasksAlive--;
                vTaskDelete(NULL);
            }
        }
        else {
            if (eTaskGetState(xTaskEchoClient2) == eDeleted)
            {
                console_print("MessageBuffer - Echo server %d terminated.\n", echoNumber);
                xTasksAlive--;
                vTaskDelete(NULL);
            }
        }

        /* Has any data been sent by the client? */
        xReceivedLength = xMessageBufferReceive(xMessageBuffers.xEchoClientBuffer, (void*)pcReceivedString, mbMESSAGE_BUFFER_LENGTH_BYTES, portMAX_DELAY);

        /* Should always receive data as max delay was used. */
        configASSERT(xReceivedLength > 0);

        console_print("MessageBuffer - Echo Server %d received message \"%s\"\n", echoNumber, pcReceivedString);

        /* Echo the received data back to the client. */
        xMessageBufferSend(xMessageBuffers.xEchoServerBuffer, (void*)pcReceivedString, xReceivedLength, portMAX_DELAY);

        console_print("MessageBuffer - Echo Server %d sent back message \"%s\"\n", echoNumber, pcReceivedString);
    }
}
/*-----------------------------------------------------------*/

BaseType_t xAreMessageBufferTasksStillRunning(void)
{
    BaseType_t xReturn = pdTRUE;

    return xReturn;
}

BaseType_t xAreMessageBuffersAlive(void) {
    return xTasksAlive != 0;
}

/*-----------------------------------------------------------*/