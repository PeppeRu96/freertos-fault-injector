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

/*
 * This version of PollQ. c is for use on systems that have limited stack
 * space and no display facilities.  The complete version can be found in
 * the Demo/Common/Full directory.
 *
 * Creates two tasks that communicate over a single queue.  One task acts as a
 * producer, the other a consumer.
 *
 * The producer loops for three iteration, posting an incrementing number onto the
 * queue each cycle.  It then delays for a fixed period before doing exactly the
 * same again.
 *
 * The consumer loops emptying the queue.  Each item removed from the queue is
 * checked to ensure it contains the expected value.  When the queue is empty it
 * blocks for a fixed period, then does the same again.
 *
 * All queue access is performed without blocking.  The consumer completely empties
 * the queue each time it runs so the producer should never find the queue full.
 *
 * An error is flagged if the consumer obtains an unexpected value or the producer
 * find the queue is full.
 */

/*
 * Changes from V2.0.0
 *
 + Delay periods are now specified using variables and constants of
 +    TickType_t rather than uint32_t.
 */

#include <stdlib.h>

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Demo program include files. */
#include "PollQ.h"

#define pollqSTACK_SIZE           configMINIMAL_STACK_SIZE
#define pollqQUEUE_SIZE           ( 10 )
#define pollqPRODUCER_DELAY       ( pdMS_TO_TICKS( ( TickType_t ) 200 ) )
#define pollqCONSUMER_DELAY       ( pollqPRODUCER_DELAY - ( TickType_t ) ( 20 / portTICK_PERIOD_MS ) )
#define pollqNO_DELAY             ( ( TickType_t ) 0 )
#define pollqVALUES_TO_PRODUCE    ( ( BaseType_t ) 3 )
#define pollqINITIAL_VALUE        ( ( BaseType_t ) 0 )

#define pollqITERATIONS           ( ( BaseType_t ) 100 )

/* The task that posts the incrementing number onto the queue. */
static portTASK_FUNCTION_PROTO( vPolledQueueProducer, pvParameters );

/* The task that empties the queue. */
static portTASK_FUNCTION_PROTO( vPolledQueueConsumer, pvParameters );

/* Variables that are used to check that the tasks are still running with no
 * errors. */
static volatile BaseType_t xPollingConsumerCount = pollqINITIAL_VALUE, xPollingProducerCount = pollqINITIAL_VALUE;
static volatile BaseType_t xPollingTasksAlive = pdTRUE;

/* TaskHandle_t for tasks */
static TaskHandle_t xTaskQConsNB, xTaskQProdNB;

/*-----------------------------------------------------------*/

void vStartPolledQueueTasks( UBaseType_t uxPriority )
{
    static QueueHandle_t xPolledQueue;

    /* Create the queue used by the producer and consumer. */
    xPolledQueue = xQueueCreate( pollqQUEUE_SIZE, ( UBaseType_t ) sizeof( uint16_t ) );

    if( xPolledQueue != NULL )
    {
        /* vQueueAddToRegistry() adds the queue to the queue registry, if one is
         * in use.  The queue registry is provided as a means for kernel aware
         * debuggers to locate queues and has no purpose if a kernel aware debugger
         * is not being used.  The call to vQueueAddToRegistry() will be removed
         * by the pre-processor if configQUEUE_REGISTRY_SIZE is not defined or is
         * defined to be less than 1. */
        vQueueAddToRegistry( xPolledQueue, "Poll_Test_Queue" );

        /* Spawn the producer and consumer. */
        // xTaskCreate( vPolledQueueConsumer, "QConsNB", pollqSTACK_SIZE, ( void * ) &xPolledQueue, uxPriority, &xTaskQConsNB );
        // xTaskCreate( vPolledQueueProducer, "QProdNB", pollqSTACK_SIZE, ( void * ) &xPolledQueue, uxPriority, &xTaskQProdNB );

        xTaskCreate(vPolledQueueConsumer, "QConsNB", pollqSTACK_SIZE, (void*)&xPolledQueue, 0, &xTaskQConsNB);
        xTaskCreate(vPolledQueueProducer, "QProdNB", pollqSTACK_SIZE, (void*)&xPolledQueue, 0, &xTaskQProdNB);

        /* log the queue handle */
        log_struct("PollQ_Queue", TYPE_QUEUE_HANDLE, xPolledQueue);

        /* log the task handles */
        log_struct("PollQ_TaskQConsNB", TYPE_TASK_HANDLE, xTaskQConsNB);
        log_struct("PollQ_TaskQProdNB", TYPE_TASK_HANDLE, xTaskQProdNB);
    }
}
/*-----------------------------------------------------------*/

static portTASK_FUNCTION( vPolledQueueProducer, pvParameters )
{
    uint16_t usValue = ( uint16_t ) 0;
    BaseType_t xError = pdFALSE, xLoop;
    BaseType_t xIterations;

    for( ; ; )
    {
        for ( xIterations = 0; xIterations < pollqITERATIONS; xIterations++ )
        {
            for( xLoop = 0; xLoop < pollqVALUES_TO_PRODUCE; xLoop++ )
            {
                /* Send an incrementing number on the queue without blocking. */
                if( xQueueSend( *( ( QueueHandle_t * ) pvParameters ), ( void * ) &usValue, pollqNO_DELAY ) != pdPASS )
                {
                    /* We should never find the queue full so if we get here there
                     * has been an error. */
                    xError = pdTRUE;

                    console_print("PollQ (It: %d, LN: %d) - ERROR: Producer found the queue full while attempting to write value %d!\n", xIterations, __LINE__, usValue);
                }
                else
                {
                    if( xError == pdFALSE )
                    {
                        /* If an error has ever been recorded we stop incrementing the
                         * check variable. */
                        portENTER_CRITICAL();
                        xPollingProducerCount++;
                        portEXIT_CRITICAL();
                    }

                    console_print("PollQ (It: %d, LN: %d) - Producer writes value %d on the queue.\n", xIterations, __LINE__, usValue);

                    /* Update the value we are going to post next time around. */
                    usValue++;
                }
            }

            /* Wait before we start posting again to ensure the consumer runs and
             * empties the queue. */
            vTaskDelay( pollqPRODUCER_DELAY );
        }
        
        console_print("PollQ (LN: %d) - Producer terminated.\n", __LINE__);
        vTaskDelete( NULL );
    }
} /*lint !e818 Function prototype must conform to API. */
/*-----------------------------------------------------------*/

static portTASK_FUNCTION( vPolledQueueConsumer, pvParameters )
{
    uint16_t usData, usExpectedValue = ( uint16_t ) 0;
    BaseType_t xError = pdFALSE;

    for( ; ; )
    {
        /* Loop until the queue is empty. */
        while( uxQueueMessagesWaiting( *( ( QueueHandle_t * ) pvParameters ) ) )
        {
            if( xQueueReceive( *( ( QueueHandle_t * ) pvParameters ), &usData, pollqNO_DELAY ) == pdPASS )
            {
                if( usData != usExpectedValue )
                {
                    /* This is not what we expected to receive so an error has
                     * occurred. */
                    xError = pdTRUE;

                    console_print("PollQ (LN: %d) - ERROR: Consumer reads value %d while expecting value %d!\n", __LINE__, usData, usExpectedValue);

                    /* Catch-up to the value we received so our next expected
                     * value should again be correct. */
                    usExpectedValue = usData;
                }
                else
                {
                    if( xError == pdFALSE )
                    {
                        /* Only increment the check variable if no errors have
                         * occurred. */
                        portENTER_CRITICAL();
                        xPollingConsumerCount++;
                        portEXIT_CRITICAL();
                    }

                    console_print("PollQ (LN: %d) - Consumer reads value %d as expected.\n", __LINE__, usData);
                }

                /* Next time round we would expect the number to be one higher. */
                usExpectedValue++;
            }
            else
            {
                console_print("PollQ (LN: %d) - ERROR: Unable to read from the queue even if the queue has some values in it!\n", __LINE__);
            }
        }

        if (eTaskGetState(xTaskQProdNB) == eDeleted)
        {
            console_print("PollQ (LN: %d) - Consumer terminated.\n", __LINE__);
            xPollingTasksAlive = pdFALSE;
            vQueueDelete(*( ( QueueHandle_t * ) pvParameters ));
            vTaskDelete(NULL);
        }

        /* Now the queue is empty we block, allowing the producer to place more
         * items in the queue. */
        vTaskDelay( pollqCONSUMER_DELAY );
    }
} /*lint !e818 Function prototype must conform to API. */
/*-----------------------------------------------------------*/

/* This is called to check that all the created tasks are still running with no errors. */
BaseType_t xArePollingQueuesStillRunning( void )
{
    BaseType_t xReturn;

    /* Check both the consumer and producer poll count to check they have both
     * been changed since out last trip round.  We do not need a critical section
     * around the check variables as this is called from a higher priority than
     * the other tasks that access the same variables. */
    if( ( xPollingConsumerCount == pollqINITIAL_VALUE ) ||
        ( xPollingProducerCount == pollqINITIAL_VALUE )
        )
    {
        xReturn = pdFALSE;
    }
    else
    {
        xReturn = pdTRUE;
    }

    /* Set the check variables back down so we know if they have been
     * incremented the next time around. */
    xPollingConsumerCount = pollqINITIAL_VALUE;
    xPollingProducerCount = pollqINITIAL_VALUE;

    return xReturn;
}

BaseType_t xArePollingQueuesAlive(void)
{
    return xPollingTasksAlive;
}
