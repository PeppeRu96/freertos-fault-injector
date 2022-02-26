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
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

 /* Standard includes. */
#include "stdio.h"
#include "string.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "stream_buffer.h"

/* Demo app includes. */
#include "StreamBufferDemo.h"

/* The number of bytes of storage in the stream buffers used in this test. */
#define sbSTREAM_BUFFER_LENGTH_BYTES    ( ( size_t ) 104 )

/* Stream buffer length one. */
#define sbSTREAM_BUFFER_LENGTH_ONE      ( ( size_t ) 1 )

/* Start and end ASCII characters used in data sent to the buffers. */
#define sbASCII_SPACE                   32
#define sbASCII_TILDA                   126

/* Defines the number of tasks to create in this test and demo. */
#define sbNUMBER_OF_ECHO_CLIENTS        ( 2 )
#define sbNUMBER_OF_SENDER_TASKS        ( 2 )

/* Priority of the test tasks.  The send and receive go from low to high
 * priority tasks, and from high to low priority tasks. */
#define sbLOWER_PRIORITY                ( (UBaseType_t) 11U )
#define sbHIGHER_PRIORITY               ( (UBaseType_t) 16U )

 /* Block times used when sending and receiving from the stream buffers. */
#define sbRX_TX_BLOCK_TIME              pdMS_TO_TICKS( 125UL )

/* A block time of 0 means "don't block". */
#define sbDONT_BLOCK                    ( 0 )

/* The trigger level sets the number of bytes that must be present in the
 * stream buffer before a task that is blocked on the stream buffer is moved out of
 * the Blocked state so it can read the bytes. */
#define sbTRIGGER_LEVEL_1               ( 1 )

 /* The size of the stack allocated to the tasks that run as part of this demo/
  * test.  The stack size is over generous in most cases. */
#ifndef configSTREAM_BUFFER_SENDER_TASK_STACK_SIZE
#define sbSTACK_SIZE    ( configMINIMAL_STACK_SIZE + ( configMINIMAL_STACK_SIZE >> 1 ) )
#else
#define sbSTACK_SIZE    configSTREAM_BUFFER_SENDER_TASK_STACK_SIZE
#endif

#ifndef configSTREAM_BUFFER_SMALLER_TASK_STACK_SIZE
#define sbSMALLER_STACK_SIZE    sbSTACK_SIZE
#else
#define sbSMALLER_STACK_SIZE    configSTREAM_BUFFER_SMALLER_TASK_STACK_SIZE
#endif

#define sbPRODUCER_DELAY       ( pdMS_TO_TICKS( ( TickType_t ) 200 ) )
#define sbCONSUMER_DELAY       ( sbPRODUCER_DELAY - ( TickType_t ) ( 20 / portTICK_PERIOD_MS ) )

  /*-----------------------------------------------------------*/

  /*
   * Tests sending and receiving various lengths of data via a stream buffer.
   * The echo client sends the data to the echo server, which then sends the
   * data back to the echo client, which checks it receives exactly what it
   * sent.
   */
static void prvEchoClient(void* pvParameters);
static void prvEchoServer(void* pvParameters);

/*
 * Tasks that send and receive to a stream buffer at a low priority and without
 * blocking, so the send and receive functions interleave in time as the tasks
 * are switched in and out.
 */
static void prvNonBlockingReceiverTask(void* pvParameters);
static void prvNonBlockingSenderTask(void* pvParameters);

/* Performs an assert() like check in a way that won't get removed when
 * performing a code coverage analysis. */
static void prvCheckExpectedState(BaseType_t xState);

/*
 * A task that creates a stream buffer with a specific trigger level, then
 * receives a string from an interrupt (the RTOS tick hook) byte by byte to
 * check it is only unblocked when the specified trigger level is reached.
 */
static void prvInterruptTriggerLevelTest(void* pvParameters);

/* The +1 is to make the test logic easier as the function that calculates the
 * free space will return one less than the actual free space - adding a 1 to the
 * actual length makes it appear to the tests as if the free space is returned as
 * it might logically be expected.  Returning 1 less than the actual free space is
 * fine as it can never result in an overrun. */
static uint8_t ucBufferStorage[sbNUMBER_OF_SENDER_TASKS][sbSTREAM_BUFFER_LENGTH_BYTES + 1];

/*-----------------------------------------------------------*/

/* The buffers used by the echo client and server tasks. */
typedef struct ECHO_STREAM_BUFFERS
{
    /* Handles to the data structures that describe the stream buffers. */
    StreamBufferHandle_t xEchoClientBuffer;
    StreamBufferHandle_t xEchoServerBuffer;

    uint32_t clientNumber;
} EchoStreamBuffers_t;
static volatile uint32_t ulEchoLoopCounters[sbNUMBER_OF_ECHO_CLIENTS + 1] = { 0 };

/* The non-blocking tasks monitor their operation, and if no errors have been
 * found, increment ulNonBlockingRxCounter.  xAreStreamBufferTasksStillRunning()
 * then checks ulNonBlockingRxCounter and only returns pdPASS if
 * ulNonBlockingRxCounter is still incrementing. */
static volatile uint32_t ulNonBlockingRxCounter = 0;

/* The task that receives characters from the tick interrupt in order to test
 * different trigger levels monitors its own behaviour.  If it has not detected any
 * error then it increments ulInterruptTriggerCounter to indicate to the check task
 * that it is still operating correctly. */
static volatile uint32_t ulInterruptTriggerCounter = 0UL;

/* The stream buffer used from the tick interrupt.  This sends one byte at a time
 * to a test task to test the trigger level operation.  The variable is set to NULL
 * in between test runs. */
static volatile StreamBufferHandle_t xInterruptStreamBuffer = NULL;

/* The data sent from the tick interrupt to the task that tests the trigger
 * level functionality. */
static const char* pcDataSentFromInterrupt = "0123456789";

/* Data that is longer than the buffer that is sent to the buffers as a stream
 * of bytes.  Parts of which are written to the stream buffer to test writing
 * different lengths at different offsets, to many bytes, part streams, streams
 * that wrap, etc..  Two messages are defined to ensure left over data is not
 * accidentally read out of the buffer. */
static const char* pc55ByteString = "One two three four five six seven eight nine ten eleven twelve thirteen";
static const char* pc54ByteString = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Maecenas molestie convallis lectus id blandit. Sed quis diam maximus, mollis massa sed, gravida arcu. Proin vestibulum erat vel eleifend proin"; //200 bytes

/* Used to log the status of the tests contained within this file for reporting
 * to a monitoring task ('check' task). */
static BaseType_t xErrorStatus = pdPASS;

static TaskHandle_t xTaskEchoServerH, xTaskEchoServerL, xNoBlockSend, xNoBlockReceive, xTriggerTask, xStaticSend1, xStaticSend2;

static uint32_t xTimesToExchangeString = 10;

static uint32_t xMaxInterruptIterations = 20;

#define MAX_LENGTH (100) 

static uint32_t xAliveTasks = sbNUMBER_OF_ECHO_CLIENTS + sbNUMBER_OF_SENDER_TASKS + 2 + 1;

/*-----------------------------------------------------------*/

void vStartStreamBufferTasks(void)
{
    StreamBufferHandle_t xStreamBuffer;
    uint32_t echoNumber;

    /* The echo servers sets up the stream buffers before creating the echo
     * client tasks.  One set of tasks has the server as the higher priority, and
     * the other has the client as the higher priority. */

    echoNumber = 1;
    // xTaskCreate(prvEchoServer, "1StrEchoServer", sbSMALLER_STACK_SIZE, (void*)echoNumber, sbHIGHER_PRIORITY, &xTaskEchoServerH);
    xTaskCreate(prvEchoServer, "1StrEchoServer", sbSMALLER_STACK_SIZE, (void*)echoNumber, 16, &xTaskEchoServerH);

    echoNumber = 2;
    //xTaskCreate( prvEchoServer, "2StrEchoServer", sbSMALLER_STACK_SIZE, (void*)echoNumber, sbLOWER_PRIORITY, &xTaskEchoServerL );

    /* The non blocking tasks run continuously and will interleave with each
     * other, so must be created at the lowest priority.  The stream buffer they
     * use is created and passed in using the task's parameter. */
    xStreamBuffer = xStreamBufferCreate(sbSTREAM_BUFFER_LENGTH_BYTES, sbTRIGGER_LEVEL_1);
    //xTaskCreate( prvNonBlockingReceiverTask, "StrNonBlkRx", configMINIMAL_STACK_SIZE, ( void * ) xStreamBuffer, tskIDLE_PRIORITY, &xNoBlockReceive );
    //xTaskCreate( prvNonBlockingSenderTask, "StrNonBlkTx", configMINIMAL_STACK_SIZE, ( void * ) xStreamBuffer, tskIDLE_PRIORITY+1, &xNoBlockSend );

    /* The task that receives bytes from an interrupt to test that it unblocks
     * at a specific trigger level must run at a high priority to minimise the risk
     * of it receiving more characters before it can execute again after being
     * unblocked. */
     //xTaskCreate( prvInterruptTriggerLevelTest, "StrTrig", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, &xTriggerTask );

     /* log the task handles */
    log_struct("StreamBuffer_TaskEchoServerHighPriority", TYPE_TASK_HANDLE, xTaskEchoServerH);
    //log_struct("StreamBuffer_TaskEchoServerLowPriority", TYPE_TASK_HANDLE, xTaskEchoServerL);
    //log_struct("StreamBuffer_TaskNoBlockingReceive", TYPE_TASK_HANDLE, xNoBlockReceive);
    //log_struct("StreamBuffer_TaskNoBlockingSend", TYPE_TASK_HANDLE, xNoBlockSend);
    //log_struct("StreamBuffer_TaskTrigger", TYPE_TASK_HANDLE, xTriggerTask);

    /* log the stream buffer handle */
    //log_struct("StreamBuffer_StreamBuffer", TYPE_STREAM_BUFFER_HANDLE, xStreamBuffer);
}
/*-----------------------------------------------------------*/

static void prvCheckExpectedState(BaseType_t xState)
{
    configASSERT(xState);

    if (xState == pdFAIL)
    {
        xErrorStatus = pdFAIL;
    }
}
/*-----------------------------------------------------------*/

static void prvNonBlockingSenderTask(void* pvParameters)
{
    StreamBufferHandle_t xStreamBuffer;
    size_t xNextChar = 0, xBytesToSend, xBytesActuallySent;
    const size_t xStringLength = strlen(pc54ByteString);

    uint32_t xTimesSentString = 1;

    /* In this case the stream buffer has already been created and is passed
     * into the task using the task's parameter. */
    xStreamBuffer = (StreamBufferHandle_t)pvParameters;

    /* Keep sending the string to the stream buffer as many bytes as possible in
     * each go.  Doesn't block so calls can interleave with the non-blocking
     * receives performed by prvNonBlockingReceiverTask(). */
    for (; ; )
    {
        /* The whole string cannot be sent at once, so xNextChar is an index to
         * the position within the string that has been sent so far.  How many
         * bytes are there left to send before the end of the string? */
        xBytesToSend = xStringLength - xNextChar;

        /* Attempt to send right up to the end of the string. */
        xBytesActuallySent = xStreamBufferSend(xStreamBuffer, (const void*)&(pc54ByteString[xNextChar]), xBytesToSend, sbDONT_BLOCK);
        prvCheckExpectedState(xBytesActuallySent <= xBytesToSend);

        if (xBytesActuallySent != 0)
            console_print("STREAMBUFFER (%d of %d): Sent %d bytes of the message (%d - %d)\n", xTimesSentString, xTimesToExchangeString, xBytesActuallySent, xNextChar, xNextChar + xBytesActuallySent);
        else
            vTaskDelay(sbPRODUCER_DELAY);

        /* Move the index up the string to the next character to be sent,
         * wrapping if the end of the string has been reached. */
        xNextChar += xBytesActuallySent;
        prvCheckExpectedState(xNextChar <= xStringLength);

        if (xNextChar == xStringLength)
        {
            xTimesSentString++;
            xNextChar = 0;

            if (xTimesSentString > xTimesToExchangeString) {
                console_print("STREAMBUFFER: sender task stopped.\n");
                xAliveTasks--;
                vTaskDelete(NULL);
            }
        }
    }
}
/*-----------------------------------------------------------*/

static void prvNonBlockingReceiverTask(void* pvParameters)
{
    StreamBufferHandle_t xStreamBuffer;
    size_t xNextChar = 0, xReceiveLength, xBytesToTest, xStartIndex;
    const size_t xStringLength = strlen(pc54ByteString);
    char cRxString[12]; /* Holds received characters. */
    BaseType_t xNonBlockingReceiveError = pdFALSE;

    /* In this case the stream buffer has already been created and is passed
     * into the task using the task's parameter. */
    xStreamBuffer = (StreamBufferHandle_t)pvParameters;

    uint32_t xTimesReceivedString = 1;

    /* Expects to receive the pc54ByteString over and over again.  Sends and
     * receives are not blocking so will interleave. */
    for (; ; )
    {
        /* Attempt to receive as many bytes as possible, up to the limit of the
         * Rx buffer size. */
        xReceiveLength = xStreamBufferReceive(xStreamBuffer, (void*)cRxString, sizeof(cRxString), sbDONT_BLOCK);

        if (xReceiveLength > 0)
        {
            /* xNextChar is the index into pc54ByteString that has been received
             * already.  If xReceiveLength bytes are added to that, will it go off
             * the end of the string?  If so, then first test up to the end of the
             * string, then go back to the start of pc54ByteString to test the
             * remains of the received data. */
            xBytesToTest = xReceiveLength;
            console_print("STREAMBUFFER (%d of %d): Received %d bytes of the message\n", xTimesReceivedString, xTimesToExchangeString, xReceiveLength);

            if ((xNextChar + xBytesToTest) > xStringLength)
            {
                /* Cap to test the received data to the end of the string. */
                xBytesToTest = xStringLength - xNextChar;

                if (memcmp((const void*)&(pc54ByteString[xNextChar]), (const void*)cRxString, xBytesToTest) != 0)
                {
                    console_print("STREAMBUFFER - ERROR: Received the wrong bytes of the message!\n");
                    xNonBlockingReceiveError = pdTRUE;
                }

                /* Then move back to the start of the string to test the
                 * remaining received bytes. */
                xNextChar = 0;
                xStartIndex = xBytesToTest;
                xBytesToTest = xReceiveLength - xBytesToTest;
            }
            else
            {
                /* The string didn't wrap in the buffer, so start comparing from
                 * the start of the received data. */
                xStartIndex = 0;
            }

            /* Test the received bytes are as expected, then move the index
             * along the string to the next expected char to receive. */
            if (memcmp((const void*)&(pc54ByteString[xNextChar]), (const void*)&(cRxString[xStartIndex]), xBytesToTest) != 0)
            {
                console_print("STREAMBUFFER - ERROR: Received the wrong bytes of the message!\n");
                xNonBlockingReceiveError = pdTRUE;
            }

            if (xNonBlockingReceiveError == pdFALSE)
            {
                /* No errors detected so increment the counter that lets the
                 *  check task know this test is still functioning correctly. */
                ulNonBlockingRxCounter++;
            }

            xNextChar += xBytesToTest;

            if (xNextChar >= xStringLength)
            {
                xTimesReceivedString++;
                xNextChar = 0;

                if (xTimesReceivedString > xTimesToExchangeString) {
                    console_print("STREAMBUFFER: receiver task stopped.\n");
                    xAliveTasks--;
                    vTaskDelete(NULL);
                }
            }
        }
        else {
            vTaskDelay(sbCONSUMER_DELAY);
        }
    }
}

/*-----------------------------------------------------------*/

static void prvEchoClient(void* pvParameters)
{
    size_t xSendLength = 0, ux;
    char* pcStringToSend, * pcStringReceived, cNextChar = sbASCII_SPACE;
    const TickType_t xTicksToWait = pdMS_TO_TICKS(50);
    StreamBufferHandle_t xTempStreamBuffer;

    /* Pointers to the client and server stream buffers are passed into this task
     * using the task's parameter. */
    EchoStreamBuffers_t* pxStreamBuffers = (EchoStreamBuffers_t*)pvParameters;

    /* Prevent compiler warnings. */
    (void)pvParameters;

    uint32_t echoNumber = pxStreamBuffers->clientNumber;


    UBaseType_t uxIndex = echoNumber;

    BaseType_t xIterations;

    /* Create the buffer into which strings to send to the server will be
     * created, and the buffer into which strings echoed back from the server will
     * be copied. */
    pcStringToSend = (char*)pvPortMalloc(sbSTREAM_BUFFER_LENGTH_BYTES);
    pcStringReceived = (char*)pvPortMalloc(sbSTREAM_BUFFER_LENGTH_BYTES);

    configASSERT(pcStringToSend);
    configASSERT(pcStringReceived);

    for (; ; )
    {
        for (xIterations = 0; xIterations < 4; xIterations++) {
            xSendLength = 0;
            ulEchoLoopCounters[uxIndex] = 0;

            while (ulEchoLoopCounters[uxIndex] != MAX_LENGTH) {
                /* Generate the length of the next string to send. */
                xSendLength++;

                /* The stream buffer is being used to hold variable length data, so
                 * each data item requires sizeof( size_t ) bytes to hold the data's
                 * length, hence the sizeof() in the if() condition below. */
                if (xSendLength > (sbSTREAM_BUFFER_LENGTH_BYTES - sizeof(size_t)))
                {
                    /* Back to a string length of 1. */
                    xSendLength = sizeof(char);
                }

                console_print("STREAMBUFFER (IT %d): Echo Client %d generating string of length %d\n", xIterations, echoNumber, xSendLength);

                memset(pcStringToSend, 0x00, sbSTREAM_BUFFER_LENGTH_BYTES);

                for (ux = 0; ux < xSendLength; ux++)
                {
                    pcStringToSend[ux] = cNextChar;

                    cNextChar++;

                    if (cNextChar > sbASCII_TILDA)
                    {
                        cNextChar = sbASCII_SPACE;
                    }
                }

                /* Send the generated string to the buffer. */
                do
                {
                    ux = xStreamBufferSend(pxStreamBuffers->xEchoClientBuffer, (void*)pcStringToSend, xSendLength, xTicksToWait);
                } while (ux == 0);

                console_print("STREAMBUFFER (IT %d): Echo Client %d sent \"%s\"\n", xIterations, echoNumber, pcStringToSend);

                /* Wait for the string to be echoed back. */
                memset(pcStringReceived, 0x00, sbSTREAM_BUFFER_LENGTH_BYTES);
                xStreamBufferReceive(pxStreamBuffers->xEchoServerBuffer, (void*)pcStringReceived, xSendLength, portMAX_DELAY);

                prvCheckExpectedState(strcmp(pcStringToSend, pcStringReceived) == 0);

                if (strcmp(pcStringToSend, pcStringReceived) != 0) {
                    console_print("STREAMBUFFER (IT %d) - ERROR: Echo Client %d received back \"%s\" instead of \"%s\"\n", xIterations, echoNumber, pcStringReceived, pcStringToSend);
                }
                else {
                    console_print("STREAMBUFFER (IT %d): Echo Client %d received back \"%s\"\n", xIterations, echoNumber, pcStringReceived);
                }

                /* Maintain a count of the number of times this code executes so a
                 * check task can determine if this task is still functioning as
                 * expected or not.  As there are two client tasks, and the priorities
                 * used are 0 and 1, the task's priority is used as an index into the
                 * loop count array. */
                ulEchoLoopCounters[uxIndex]++;
            }

        }



        console_print("STREAMBUFFER: Echo Client %d stopped.\n", echoNumber);
        xAliveTasks--;
        vTaskDelete(NULL);

    }
}
/*-----------------------------------------------------------*/

static void prvEchoServer(void* pvParameters)
{
    size_t xReceivedLength;
    char* pcReceivedString;
    EchoStreamBuffers_t xStreamBuffers;
    TickType_t xTimeOnEntering;
    const TickType_t xTicksToBlock = pdMS_TO_TICKS(350UL);

    uint32_t receivedMsgs = 0;

    BaseType_t xIteration = 0;

    uint32_t echoNumber = (uint32_t)pvParameters;

    /* Create the stream buffer used to send data from the client to the server,
     * and the stream buffer used to echo the data from the server back to the
     * client. */
    xStreamBuffers.xEchoClientBuffer = xStreamBufferCreate(sbSTREAM_BUFFER_LENGTH_BYTES, sbTRIGGER_LEVEL_1);
    xStreamBuffers.xEchoServerBuffer = xStreamBufferCreate(sbSTREAM_BUFFER_LENGTH_BYTES, sbTRIGGER_LEVEL_1);
    configASSERT(xStreamBuffers.xEchoClientBuffer);
    configASSERT(xStreamBuffers.xEchoServerBuffer);

    /* assign the echoNumber to the echo client/server */
    xStreamBuffers.clientNumber = echoNumber;

    /* Create the buffer into which received strings will be copied. */
    pcReceivedString = (char*)pvPortMalloc(sbSTREAM_BUFFER_LENGTH_BYTES);
    configASSERT(pcReceivedString);

    /* Don't expect to receive anything yet! */
    xTimeOnEntering = xTaskGetTickCount();
    xReceivedLength = xStreamBufferReceive(xStreamBuffers.xEchoClientBuffer, (void*)pcReceivedString, sbSTREAM_BUFFER_LENGTH_BYTES, xTicksToBlock);
    prvCheckExpectedState(((TickType_t)(xTaskGetTickCount() - xTimeOnEntering)) >= xTicksToBlock);
    prvCheckExpectedState(xReceivedLength == 0);

    /* Now the stream buffers have been created the echo client task can be
     * created.  If this server task has the higher priority then the client task
     * is created at the lower priority - if this server task has the lower
     * priority then the client task is created at the higher priority. */
    if (uxTaskPriorityGet(NULL) == sbLOWER_PRIORITY)
    {
        xTaskCreate(prvEchoClient, "EchoClient", sbSMALLER_STACK_SIZE, (void*)&xStreamBuffers, sbLOWER_PRIORITY + 1, NULL);
    }
    else
    {
        xTaskCreate(prvEchoClient, "EchoClient", sbSMALLER_STACK_SIZE, (void*)&xStreamBuffers, sbHIGHER_PRIORITY - 1, NULL);
    }

    for (; ; )
    {
        memset(pcReceivedString, 0x00, sbSTREAM_BUFFER_LENGTH_BYTES);

        /* Has any data been sent by the client? */
        xReceivedLength = xStreamBufferReceive(xStreamBuffers.xEchoClientBuffer, (void*)pcReceivedString, sbSTREAM_BUFFER_LENGTH_BYTES, portMAX_DELAY);

        /* Should always receive data as max delay was used. */
        prvCheckExpectedState(xReceivedLength > 0);

        console_print("STREAMBUFFER (IT %d): Echo Server %d received \"%s\"\n", xIteration, echoNumber, pcReceivedString);

        /* Echo the received data back to the client. */
        xStreamBufferSend(xStreamBuffers.xEchoServerBuffer, (void*)pcReceivedString, xReceivedLength, portMAX_DELAY);

        console_print("STREAMBUFFER (IT %d): Echo Server %d sent back \"%s\"\n", xIteration, echoNumber, pcReceivedString);

        if (xReceivedLength == (sbSTREAM_BUFFER_LENGTH_BYTES - sizeof(size_t))) {
            xIteration++;
            if (xIteration == 4) {
                vTaskDelay((TickType_t)50);
                console_print("STREAMBUFFER: Echo Server %d stopped.\n", echoNumber);
                xAliveTasks--;
                vTaskDelete(NULL);
            }
        }
    }
}
/*-----------------------------------------------------------*/

static void prvInterruptTriggerLevelTest(void* pvParameters)
{
    StreamBufferHandle_t xStreamBuffer;
    size_t xTriggerLevel = 1, xBytesReceived;
    const size_t xStreamBufferSizeBytes = (size_t)9, xMaxTriggerLevel = (size_t)7, xMinTriggerLevel = (size_t)2;
    const TickType_t xReadBlockTime = 5, xCycleBlockTime = pdMS_TO_TICKS(100);
    uint8_t ucRxData[9];
    BaseType_t xErrorDetected = pdFALSE;

#ifndef configSTREAM_BUFFER_TRIGGER_LEVEL_TEST_MARGIN
    const size_t xAllowableMargin = (size_t)0;
#else
    const size_t xAllowableMargin = (size_t)configSTREAM_BUFFER_TRIGGER_LEVEL_TEST_MARGIN;
#endif

    /* Remove compiler warning about unused parameter. */
    (void)pvParameters;

    for (; ; )
    {
        for (xTriggerLevel = xMinTriggerLevel; xTriggerLevel < xMaxTriggerLevel; xTriggerLevel++)
        {
            /* This test is very time sensitive so delay at the beginning to ensure
             * the rest of the system is up and running before starting.  Delay between
             * each loop to ensure the interrupt that sends to the stream buffer
             * detects it needs to start sending from the start of the strin again.. */
            vTaskDelay(xCycleBlockTime);

            /* Create the stream buffer that will be used from inside the tick
             * interrupt. */
            memset(ucRxData, 0x00, sizeof(ucRxData));
            xStreamBuffer = xStreamBufferCreate(xStreamBufferSizeBytes, xTriggerLevel);
            configASSERT(xStreamBuffer);

            /* Now the stream buffer has been created it can be assigned to the
             * file scope variable, which will allow the tick interrupt to start
             * using it. */
            taskENTER_CRITICAL();
            {
                xInterruptStreamBuffer = xStreamBuffer;
            }
            taskEXIT_CRITICAL();

            xBytesReceived = xStreamBufferReceive(xStreamBuffer, (void*)ucRxData, sizeof(ucRxData), xReadBlockTime);

            /* Set the file scope variable back to NULL so the interrupt doesn't
             * try to use it again. */
            taskENTER_CRITICAL();
            {
                xInterruptStreamBuffer = NULL;
            }
            taskEXIT_CRITICAL();

            /* Now check the number of bytes received equals the trigger level,
             * except in the case that the read timed out before the trigger level
             * was reached. */
            if (xTriggerLevel > xReadBlockTime)
            {
                /* Trigger level was greater than the block time so expect to
                 * time out having received xReadBlockTime bytes. */
                if (xBytesReceived > xReadBlockTime)
                {
                    /* Received more bytes than expected.  That could happen if
                     * this task unblocked at the right time, but an interrupt
                     * added another byte to the stream buffer before this task was
                     * able to run. */
                    if ((xBytesReceived - xReadBlockTime) > xAllowableMargin)
                    {
                        console_print("STREAMBUFFER - ERROR: error with bytes received by Interrupt!\n");
                        xErrorDetected = pdTRUE;
                    }
                }
                else if (xReadBlockTime != xBytesReceived)
                {
                    /* It is possible the interrupt placed an item in the stream
                     * buffer before this task called xStreamBufferReceive(), but
                     * if that is the case then xBytesReceived will only every be
                     * 0 as the interrupt will only have executed once. */
                    if (xBytesReceived != 1)
                    {
                        console_print("STREAMBUFFER - ERROR: error with bytes received by Interrupt!\n");
                        xErrorDetected = pdTRUE;
                    }
                }
            }
            else if (xTriggerLevel < xReadBlockTime)
            {
                /* Trigger level was less than the block time so we expect to
                 * have received the trigger level number of bytes - could be more
                 * though depending on other activity between the task being
                 * unblocked and the task reading the number of bytes received.  It
                 * could also be less if the interrupt already put something in the
                 * stream buffer before this task attempted to read it - in which
                 * case the task would have returned the available bytes immediately
                 * without ever blocking - in that case the bytes received will
                 * only ever be 1 as the interrupt would not have executed more
                 * than one in that time unless this task has too low a priority. */
                if (xBytesReceived < xTriggerLevel)
                {
                    if (xBytesReceived != 1)
                    {
                        console_print("STREAMBUFFER - ERROR: error with bytes received by Interrupt!\n");
                        xErrorDetected = pdTRUE;
                    }
                }
                else if ((xBytesReceived - xTriggerLevel) > xAllowableMargin)
                {
                    console_print("STREAMBUFFER - ERROR: error with bytes received by Interrupt!\n");
                    xErrorDetected = pdTRUE;
                }
            }
            else
            {
                /* The trigger level equalled the block time, so expect to
                 * receive no greater than the block time.  It could also be less
                 * if the interrupt already put something in the stream buffer
                 * before this task attempted to read it - in which case the task
                 * would have returned the available bytes immediately without ever
                 * blocking - in that case the bytes received would only ever be 1
                 * because the interrupt is not going to execute twice in that time
                 * unless this task is running a too low a priority. */
                if (xBytesReceived < xReadBlockTime)
                {
                    if (xBytesReceived != 1)
                    {
                        console_print("STREAMBUFFER - ERROR: error with bytes received by Interrupt!\n");
                        xErrorDetected = pdTRUE;
                    }
                }
                else if ((xBytesReceived - xReadBlockTime) > xAllowableMargin)
                {
                    console_print("STREAMBUFFER - ERROR: error with bytes received by Interrupt!\n");
                    xErrorDetected = pdTRUE;
                }
            }

            if (xBytesReceived > sizeof(ucRxData))
            {
                console_print("STREAMBUFFER - ERROR: error with bytes received by Interrupt!\n");
                xErrorDetected = pdTRUE;
            }
            else if (memcmp((void*)ucRxData, (const void*)pcDataSentFromInterrupt, xBytesReceived) != 0)
            {
                /* Received data didn't match that expected. */
                xErrorDetected = pdTRUE;
                console_print("STREAMBUFFER - ERROR: wrong bytes received by Interrupt!\n");
            }

            if (xErrorDetected == pdFALSE)
            {
                console_print("STREAMBUFFER: correctly received \"%s\" from Interrupt\n", ucRxData);

            }

            /* Tidy up ready for the next loop. */
            vStreamBufferDelete(xStreamBuffer);
        }

        /* Increment the cycle counter so the 'check' task knows this test
                 * is still running without error. */
        ulInterruptTriggerCounter++;

        if (ulInterruptTriggerCounter == xMaxInterruptIterations) {
            console_print("STREAMBUFFER: Interrupt task stopped.\n");
            xAliveTasks--;
            vTaskDelete(NULL);
        }
    }
}
/*-----------------------------------------------------------*/

BaseType_t xAreStreamBufferTasksStillRunning(void)
{
    static uint32_t ulLastEchoLoopCounters[sbNUMBER_OF_ECHO_CLIENTS] = { 0 };
    static uint32_t ulLastNonBlockingRxCounter = 0;
    static uint32_t ulLastInterruptTriggerCounter = 0;
    BaseType_t x;

    for (x = 0; x < sbNUMBER_OF_ECHO_CLIENTS; x++)
    {
        if (ulLastEchoLoopCounters[x] == ulEchoLoopCounters[x])
        {
            xErrorStatus = pdFAIL;
        }
        else
        {
            ulLastEchoLoopCounters[x] = ulEchoLoopCounters[x];
        }
    }

    if (ulNonBlockingRxCounter == ulLastNonBlockingRxCounter)
    {
        xErrorStatus = pdFAIL;
    }
    else
    {
        ulLastNonBlockingRxCounter = ulNonBlockingRxCounter;
    }

    if (ulLastInterruptTriggerCounter == ulInterruptTriggerCounter)
    {
        xErrorStatus = pdFAIL;
    }
    else
    {
        ulLastInterruptTriggerCounter = ulInterruptTriggerCounter;
    }

    return xErrorStatus;
}
/*-----------------------------------------------------------*/

BaseType_t xAreStreamBuffersAlive(void) {
    return xAliveTasks != 0;
}
