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
#define mbNUMBER_OF_SENDER_TASKS           ( 2 )

/* Priority of the test tasks.  The send and receive go from low to high
 * priority tasks, and from high to low priority tasks. */
#define mbLOWER_PRIORITY                   ( tskIDLE_PRIORITY )
#define mbHIGHER_PRIORITY                  ( tskIDLE_PRIORITY + 1 )

/* Block times used when sending and receiving from the message buffers. */
#define mbRX_TX_BLOCK_TIME                 pdMS_TO_TICKS( 175UL )

/* A block time of 0 means "don't block". */
#define mbDONT_BLOCK                       ( 0 )

/*-----------------------------------------------------------*/


/*
 * Tests sending and receiving various lengths of messages via a message buffer.
 * The echo client sends the messages to the echo server, which then sends the
 * message back to the echo client which, checks it receives exactly what it
 * sent.
 */
static void prvEchoClient( void * pvParameters );
static void prvEchoServer( void * pvParameters );

/*
 * Tasks that send and receive to a message buffer at a low priority and without
 * blocking, so the send and receive functions interleave in time as the tasks
 * are switched in and out.
 */
static void prvNonBlockingReceiverTask( void * pvParameters );
static void prvNonBlockingSenderTask( void * pvParameters );

#if ( configSUPPORT_STATIC_ALLOCATION == 1 )

/* This file tests both statically and dynamically allocated message buffers.
 * Allocate the structures and buffers to be used by the statically allocated
 * objects, which get used in the echo tests. */
    static void prvReceiverTask( void * pvParameters );
    static void prvSenderTask( void * pvParameters );

    static StaticMessageBuffer_t xStaticMessageBuffers[ mbNUMBER_OF_ECHO_CLIENTS ];
    static uint8_t ucBufferStorage[ mbNUMBER_OF_SENDER_TASKS ][ mbMESSAGE_BUFFER_LENGTH_BYTES + 1 ];
    static uint32_t ulSenderLoopCounters[ mbNUMBER_OF_SENDER_TASKS ] = { 0 };
#endif /* configSUPPORT_STATIC_ALLOCATION */


#if ( configRUN_ADDITIONAL_TESTS == 1 )
    #define mbCOHERENCE_TEST_BUFFER_SIZE                  20
    #define mbCOHERENCE_TEST_BYTES_WRITTEN                5
    #define mbBYTES_TO_STORE_MESSAGE_LENGTH               ( sizeof( configMESSAGE_BUFFER_LENGTH_TYPE ) )
    #define mbEXPECTED_FREE_BYTES_AFTER_WRITING_STRING    ( mbCOHERENCE_TEST_BUFFER_SIZE - ( mbCOHERENCE_TEST_BYTES_WRITTEN + mbBYTES_TO_STORE_MESSAGE_LENGTH ) )

    static void prvSpaceAvailableCoherenceActor( void * pvParameters );
    static void prvSpaceAvailableCoherenceTester( void * pvParameters );
    static MessageBufferHandle_t xCoherenceTestMessageBuffer = NULL;

    static uint32_t ulSizeCoherencyTestCycles = 0UL;
#endif /* if ( configRUN_ADDITIONAL_TESTS == 1 ) */

/*-----------------------------------------------------------*/

/* The buffers used by the echo client and server tasks. */
typedef struct ECHO_MESSAGE_BUFFERS
{
    /* Handles to the data structures that describe the message buffers. */
    MessageBufferHandle_t xEchoClientBuffer;
    MessageBufferHandle_t xEchoServerBuffer;
} EchoMessageBuffers_t;
static uint32_t ulEchoLoopCounters[ mbNUMBER_OF_ECHO_CLIENTS ] = { 0 };

/* The non-blocking tasks monitor their operation, and if no errors have been
 * found, increment ulNonBlockingRxCounter.  xAreMessageBufferTasksStillRunning()
 * then checks ulNonBlockingRxCounter and only returns pdPASS if
 * ulNonBlockingRxCounter is still incrementing. */
static uint32_t ulNonBlockingRxCounter = 0;

/* A message that is longer than the buffer, parts of which are written to the
 * message buffer to test writing different lengths at different offsets. */
static const char * pc55ByteString = "One two three four five six seven eight nine ten eleve";

/* Remember the required stack size so tasks can be created at run time (after
 * initialisation time. */
static configSTACK_DEPTH_TYPE xBlockingStackSize = 0;

static TaskHandle_t xTaskEchoServerH, xTaskEchoServerL, xNoBlockSend, xNoBlockReceive, xStaticSend1, xStaticSend2;

static volatile BaseType_t xTasksAlive = 6;

typedef struct ECHO_PARAMETERS
{
    EchoMessageBuffers_t echoMessageBuffers;
    uint32_t echoClientNumber;
} EchoParameters_t;

/*-----------------------------------------------------------*/

void vStartMessageBufferTasks( configSTACK_DEPTH_TYPE xStackSize )
{
    MessageBufferHandle_t xMessageBuffer;

    #ifndef configMESSAGE_BUFFER_BLOCK_TASK_STACK_SIZE
        xBlockingStackSize = ( xStackSize + ( xStackSize >> 1U ) );
    #else
        xBlockingStackSize = configMESSAGE_BUFFER_BLOCK_TASK_STACK_SIZE;
    #endif

    uint32_t echoNumber;

    /* The echo servers sets up the message buffers before creating the echo
     * client tasks.  One set of tasks has the server as the higher priority, and
     * the other has the client as the higher priority. */

    echoNumber = 1;
    xTaskCreate( prvEchoServer, "1EchoServer", xBlockingStackSize, (void*)echoNumber, mbHIGHER_PRIORITY, &xTaskEchoServerH );

    echoNumber = 2;
    xTaskCreate( prvEchoServer, "2EchoServer", xBlockingStackSize, (void*)echoNumber, mbLOWER_PRIORITY, &xTaskEchoServerL );

    /* The non blocking tasks run continuously and will interleave with each
     * other, so must be created at the lowest priority.  The message buffer they
     * use is created and passed in using the task's parameter. */
    xMessageBuffer = xMessageBufferCreate( mbMESSAGE_BUFFER_LENGTH_BYTES );
    xTaskCreate( prvNonBlockingReceiverTask, "NonBlkRx", xStackSize, ( void * ) xMessageBuffer, tskIDLE_PRIORITY, &xNoBlockReceive );
    xTaskCreate( prvNonBlockingSenderTask, "NonBlkTx", xStackSize, ( void * ) xMessageBuffer, tskIDLE_PRIORITY, &xNoBlockSend );

    /* log the task handles */
    log_struct("MessageBuffer_TaskEchoServerHighPriority", TYPE_TASK_HANDLE, xTaskEchoServerH);
    log_struct("MessageBuffer_TaskEchoServerLowPriority", TYPE_TASK_HANDLE, xTaskEchoServerL);
    log_struct("MessageBuffer_TaskNoBlockingReceive", TYPE_TASK_HANDLE, xNoBlockReceive);
    log_struct("MessageBuffer_TaskNoBlockingSend", TYPE_TASK_HANDLE, xNoBlockSend);

    /* log the message buffer handle */
    log_struct("MessageBuffer_MessageBuffer", TYPE_MESSAGE_BUFFER_HANDLE, xMessageBuffer);
}
/*-----------------------------------------------------------*/

static void prvNonBlockingSenderTask( void * pvParameters )
{
    MessageBufferHandle_t xMessageBuffer;
    int32_t iDataToSend = 0;
    size_t xStringLength;
    const int32_t iMaxValue = 400;
    char cTxString[ 12 ]; /* Large enough to hold a 32 number in ASCII. */

    /* In this case the message buffer has already been created and is passed
     * into the task using the task's parameter. */

    xMessageBuffer = ( MessageBufferHandle_t ) pvParameters;

    /* Create a string from an incrementing number.  The length of the
     * string will increase and decrease as the value of the number increases
     * then overflows. */
    memset( cTxString, 0x00, sizeof( cTxString ) );
    sprintf( cTxString, "%d", ( int ) iDataToSend );
    xStringLength = strlen( cTxString );

    for( ; ; )
    {
        /* Doesn't block so calls can interleave with the non-blocking
         * receives performed by prvNonBlockingReceiverTask(). */
        if( xMessageBufferSend( xMessageBuffer, ( void * ) cTxString, strlen( cTxString ), mbDONT_BLOCK ) == xStringLength )
        {
            console_print("MESSAGEBUFFER: Sent message %s\n", cTxString);
            iDataToSend++;

            if( iDataToSend > iMaxValue )
            {
                /* Stop the Sender Task */
                console_print("MESSAGEBUFFER: Sender task stopped.\n");
                xTasksAlive--;
                vTaskDelete(NULL);
            }

            /* Create the next string. */
            memset( cTxString, 0x00, sizeof( cTxString ) );
            sprintf( cTxString, "%d", ( int ) iDataToSend );
            xStringLength = strlen( cTxString );
        }
    }
}
/*-----------------------------------------------------------*/

static void prvNonBlockingReceiverTask( void * pvParameters )
{
    MessageBufferHandle_t xMessageBuffer;
    BaseType_t xNonBlockingReceiveError = pdFALSE;
    int32_t iDataToSend = 0;
    size_t xStringLength, xReceiveLength;
    const int32_t iMaxValue = 400;
    char cExpectedString[ 12 ]; /* Large enough to hold a 32 number in ASCII. */
    char cRxString[ 12 ];

    /* In this case the message buffer has already been created and is passed
     * into the task using the task's parameter. */
    xMessageBuffer = ( MessageBufferHandle_t ) pvParameters;

    /* Create a string from an incrementing number.  The length of the
     * string will increase and decrease as the value of the number increases
     * then overflows.  This should always match the string sent to the buffer by
     * the non blocking sender task. */
    memset( cExpectedString, 0x00, sizeof( cExpectedString ) );
    memset( cRxString, 0x00, sizeof( cRxString ) );
    sprintf( cExpectedString, "%d", ( int ) iDataToSend );
    xStringLength = strlen( cExpectedString );

    for( ; ; )
    {
        /* Doesn't block so calls can interleave with the non-blocking
         * receives performed by prvNonBlockingReceiverTask(). */
        xReceiveLength = xMessageBufferReceive( xMessageBuffer, ( void * ) cRxString, sizeof( cRxString ), mbDONT_BLOCK );

        /* Should only ever receive no data is available, or the expected
         * length of data is available. */
        if( ( xReceiveLength != 0 ) && ( xReceiveLength != xStringLength ) )
        {
            console_print("MESSAGEBUFFER - ERROR: wrong message size!\n");
            xNonBlockingReceiveError = pdTRUE;
        }

        if( xReceiveLength == xStringLength )
        {
            /* Ensure the received data was that expected, then generate the
             * next expected string. */
            if( strcmp( cRxString, cExpectedString ) != 0 )
            {
                console_print("MESSAGEBUFFER - ERROR: wrong message received!\n");
                xNonBlockingReceiveError = pdTRUE;
            }
            else
            {
                console_print("MESSAGEBUFFER: received value %s\n", cRxString);
            }

            iDataToSend++;

            if( iDataToSend > iMaxValue )
            {
                /* Stop the Receiver Task */
                console_print("MESSAGEBUFFER: Receiver task stopped.\n");
                xTasksAlive--;
                vTaskDelete(NULL);
            }

            memset( cExpectedString, 0x00, sizeof( cExpectedString ) );
            memset( cRxString, 0x00, sizeof( cRxString ) );
            sprintf( cExpectedString, "%d", ( int ) iDataToSend );
            xStringLength = strlen( cExpectedString );

            if( xNonBlockingReceiveError == pdFALSE )
            {
                /* No errors detected so increment the counter that lets the
                 *  check task know this test is still functioning correctly. */
                ulNonBlockingRxCounter++;
            }
        }
    }
}
/*-----------------------------------------------------------*/

static void prvEchoClient( void * pvParameters )
{
    size_t xSendLength = 0, ux;
    char * pcStringToSend, * pcStringReceived, cNextChar = mbASCII_SPACE;
    const TickType_t xTicksToWait = pdMS_TO_TICKS( 50 );


/* The task's priority is used as an index into the loop counters used to
 * indicate this task is still running. */
    UBaseType_t uxIndex = uxTaskPriorityGet( NULL );

/* Pointers to the client and server message buffers are passed into this task
 * using the task's parameter. */
    EchoParameters_t* parameters = (EchoParameters_t*)pvParameters;

    EchoMessageBuffers_t pxMessageBuffers = parameters->echoMessageBuffers;
    uint32_t clientNumber = parameters->echoClientNumber;

    /* Prevent compiler warnings. */
    ( void ) pvParameters;

    /* Create the buffer into which strings to send to the server will be
     * created, and the buffer into which strings echoed back from the server will
     * be copied. */
    pcStringToSend = ( char * ) pvPortMalloc( mbMESSAGE_BUFFER_LENGTH_BYTES );
    pcStringReceived = ( char * ) pvPortMalloc( mbMESSAGE_BUFFER_LENGTH_BYTES );

    configASSERT( pcStringToSend );
    configASSERT( pcStringReceived );

    for( ; ; )
    {
        /* Generate the length of the next string to send. */
        xSendLength++;
        

        /* The message buffer is being used to hold variable length data, so
         * each data item requires sizeof( size_t ) bytes to hold the data's
         * length, hence the sizeof() in the if() condition below. */
        if( xSendLength > ( mbMESSAGE_BUFFER_LENGTH_BYTES - sizeof( size_t ) ) )
        {
            console_print("MESSAGEBUFFER: Echo Client %d stopped.\n", clientNumber);
            xTasksAlive--;
            vTaskDelete(NULL);
        }


        console_print("MESSAGEBUFFER: Echo Client %d preparing to send a string of size %d\n", clientNumber, xSendLength);
        memset( pcStringToSend, 0x00, mbMESSAGE_BUFFER_LENGTH_BYTES );

        for( ux = 0; ux < xSendLength; ux++ )
        {
            pcStringToSend[ ux ] = cNextChar;

            cNextChar++;

            if( cNextChar > mbASCII_TILDA )
            {
                cNextChar = mbASCII_SPACE;
            }
        }

        /* Send the generated string to the buffer. */
        do
        {
            ux = xMessageBufferSend( pxMessageBuffers.xEchoClientBuffer, ( void * ) pcStringToSend, xSendLength, xTicksToWait );

            if( ux == 0 )
            {
                mtCOVERAGE_TEST_MARKER();
            }
        } while( ux == 0 );

        console_print("MESSAGEBUFFER: Echo Client %d sent message \"%s\"\n", clientNumber, pcStringToSend);

        /* Wait for the string to be echoed back. */
        memset( pcStringReceived, 0x00, mbMESSAGE_BUFFER_LENGTH_BYTES );
        xMessageBufferReceive( pxMessageBuffers.xEchoServerBuffer, ( void * ) pcStringReceived, xSendLength, portMAX_DELAY );

        console_print("MESSAGEBUFFER: Echo Client %d received back message \"%s\"\n", clientNumber, pcStringReceived);

        configASSERT( strcmp( pcStringToSend, pcStringReceived ) == 0 );
    }
}
/*-----------------------------------------------------------*/

static void prvEchoServer( void * pvParameters )
{
    MessageBufferHandle_t xTempMessageBuffer;
    size_t xReceivedLength;
    char * pcReceivedString;
    EchoMessageBuffers_t xMessageBuffers;
    TickType_t xTimeOnEntering;
    const TickType_t xTicksToBlock = pdMS_TO_TICKS( 250UL );

    EchoParameters_t clientParameters;

    uint32_t echoNumber = (uint32_t) pvParameters;

    /* Create the message buffer used to send data from the client to the server,
     * and the message buffer used to echo the data from the server back to the
     * client. */
    xMessageBuffers.xEchoClientBuffer = xMessageBufferCreate( mbMESSAGE_BUFFER_LENGTH_BYTES );
    xMessageBuffers.xEchoServerBuffer = xMessageBufferCreate( mbMESSAGE_BUFFER_LENGTH_BYTES );
    configASSERT( xMessageBuffers.xEchoClientBuffer );
    configASSERT( xMessageBuffers.xEchoServerBuffer );

    /* Create the buffer into which received strings will be copied. */
    pcReceivedString = ( char * ) pvPortMalloc( mbMESSAGE_BUFFER_LENGTH_BYTES );
    configASSERT( pcReceivedString );

    /* Don't expect to receive anything yet! */
    xTimeOnEntering = xTaskGetTickCount();
    xReceivedLength = xMessageBufferReceive( xMessageBuffers.xEchoClientBuffer, ( void * ) pcReceivedString, mbMESSAGE_BUFFER_LENGTH_BYTES, xTicksToBlock );
    configASSERT( ( ( TickType_t ) ( xTaskGetTickCount() - xTimeOnEntering ) ) >= xTicksToBlock );
    configASSERT( xReceivedLength == 0 );
    ( void ) xTimeOnEntering; /* In case configASSERT() is not defined. */


    clientParameters.echoMessageBuffers = xMessageBuffers;
    clientParameters.echoClientNumber = echoNumber;


    /* Now the message buffers have been created the echo client task can be
     * created.  If this server task has the higher priority then the client task
     * is created at the lower priority - if this server task has the lower
     * priority then the client task is created at the higher priority. */
    if( uxTaskPriorityGet( NULL ) == mbLOWER_PRIORITY )
    {
        xTaskCreate( prvEchoClient, "EchoClient", configMINIMAL_STACK_SIZE, ( void * ) &clientParameters, mbHIGHER_PRIORITY, NULL );
    }
    else
    {
        xTaskCreate( prvEchoClient, "EchoClient", configMINIMAL_STACK_SIZE, ( void * ) &clientParameters, mbLOWER_PRIORITY, NULL );
    }

    for( ; ; )
    {
        memset( pcReceivedString, 0x00, mbMESSAGE_BUFFER_LENGTH_BYTES );

        /* Has any data been sent by the client? */
        xReceivedLength = xMessageBufferReceive( xMessageBuffers.xEchoClientBuffer, ( void * ) pcReceivedString, mbMESSAGE_BUFFER_LENGTH_BYTES, portMAX_DELAY );

        /* Should always receive data as max delay was used. */
        configASSERT( xReceivedLength > 0 );

        console_print("MESSAGEBUFFER: Echo Server %d received message \"%s\"\n", echoNumber, pcReceivedString);

        /* Echo the received data back to the client. */
        xMessageBufferSend( xMessageBuffers.xEchoServerBuffer, ( void * ) pcReceivedString, xReceivedLength, portMAX_DELAY );

        console_print("MESSAGEBUFFER: Echo Server %d sent back message \"%s\"\n", echoNumber, pcReceivedString);

        if (xReceivedLength == (mbMESSAGE_BUFFER_LENGTH_BYTES - sizeof(size_t)) ) {
            console_print("MESSAGEBUFFER: Echo Server %d stopped.\n", echoNumber);
            xTasksAlive--;
            vTaskDelete(NULL);
        }
    }
}
/*-----------------------------------------------------------*/

BaseType_t xAreMessageBufferTasksStillRunning( void )
{
    static uint32_t ulLastEchoLoopCounters[ mbNUMBER_OF_ECHO_CLIENTS ] = { 0 };
    static uint32_t ulLastNonBlockingRxCounter = 0;
    BaseType_t xReturn = pdPASS, x;

    for( x = 0; x < mbNUMBER_OF_ECHO_CLIENTS; x++ )
    {
        if( ulLastEchoLoopCounters[ x ] == ulEchoLoopCounters[ x ] )
        {
            xReturn = pdFAIL;
        }
        else
        {
            ulLastEchoLoopCounters[ x ] = ulEchoLoopCounters[ x ];
        }
    }

    if( ulNonBlockingRxCounter == ulLastNonBlockingRxCounter )
    {
        xReturn = pdFAIL;
    }
    else
    {
        ulLastNonBlockingRxCounter = ulNonBlockingRxCounter;
    }

    return xReturn;
}

BaseType_t xAreMessageBuffersAlive(void) {
    return xTasksAlive != 0;
}

/*-----------------------------------------------------------*/
