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
  * Creates six tasks that operate on three queues as follows:
  *
  * The first two tasks send and receive an incrementing number to/from a queue.
  * One task acts as a producer and the other as the consumer.  The consumer is a
  * higher priority than the producer and is set to block on queue reads.  The queue
  * only has space for one item - as soon as the producer posts a message on the
  * queue the consumer will unblock, pre-empt the producer, and remove the item.
  *
  * The second two tasks work the other way around.  Again the queue used only has
  * enough space for one item.  This time the consumer has a lower priority than the
  * producer.  The producer will try to post on the queue blocking when the queue is
  * full.  When the consumer wakes it will remove the item from the queue, causing
  * the producer to unblock, pre-empt the consumer, and immediately re-fill the
  * queue.
  *
  * The last two tasks use the same queue producer and consumer functions.  This time the queue has
  * enough space for lots of items and the tasks operate at the same priority.  The
  * producer will execute, placing items into the queue.  The consumer will start
  * executing when either the queue becomes full (causing the producer to block) or
  * a context switch occurs (tasks of the same priority will time slice).
  *
  */

#include <stdlib.h>

  /* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Demo program include files. */
#include "BlockQ.h"

#define blckqSTACK_SIZE       configMINIMAL_STACK_SIZE
#define blckqNUM_TASK_SETS    ( 3 )

#if ( configSUPPORT_DYNAMIC_ALLOCATION == 0 )
#error This example cannot be used if dynamic allocation is not allowed.
#endif

/* Structure used to pass parameters to the blocking queue tasks. */
typedef struct BLOCKING_QUEUE_PARAMETERS
{
    QueueHandle_t xQueue;             /*< The queue to be used by the task. */
    TickType_t xBlockTime;            /*< The block time to use on queue reads/writes. */
    volatile short* psCheckVariable; /*< Incremented on each successful cycle to check the task is still running. */
    int paramNumber;
} xBlockingQueueParameters;

/* Task function that creates an incrementing number and posts it on a queue. */
static portTASK_FUNCTION_PROTO(vBlockingQueueProducer, pvParameters);

/* Task function that removes the incrementing number from a queue and checks that
 * it is the expected number. */
static portTASK_FUNCTION_PROTO(vBlockingQueueConsumer, pvParameters);

/* Variables which are incremented each time an item is removed from a queue, and
 * found to be the expected value.
 * These are used to check that the tasks are still running. */
static volatile short sBlockingConsumerCount[blckqNUM_TASK_SETS] = { (uint16_t)0, (uint16_t)0, (uint16_t)0 };

/* Variable which are incremented each time an item is posted on a queue.   These
 * are used to check that the tasks are still running. */
static volatile short sBlockingProducerCount[blckqNUM_TASK_SETS] = { (uint16_t)0, (uint16_t)0, (uint16_t)0 };
static short sBlockingTasksPAlive[blckqNUM_TASK_SETS] = { (uint16_t)1, (uint16_t)1, (uint16_t)1 }; /* producers */
static short sBlockingTasksCAlive[blckqNUM_TASK_SETS] = { (uint16_t)1, (uint16_t)1, (uint16_t)1 }; /* consumers */

/* TaskHandle_t for tasks */
static TaskHandle_t xTaskQConsB1, xTaskQProdB2, xTaskQConsB3, xTaskQProdB4, xTaskQProdB5, xTaskQConsB6;

static int receiveTask_n = 0, sendTask_n = 0;

static int limitValue = 11;

/*-----------------------------------------------------------*/

void vStartBlockingQueueTasks(UBaseType_t uxPriority)
{
    xBlockingQueueParameters* pxQueueParameters1, * pxQueueParameters2;
    xBlockingQueueParameters* pxQueueParameters3, * pxQueueParameters4;
    xBlockingQueueParameters* pxQueueParameters5, * pxQueueParameters6;
    const UBaseType_t uxQueueSize1 = 1, uxQueueSize5 = 5;
    const TickType_t xBlockTime = pdMS_TO_TICKS((TickType_t)1000);
    const TickType_t xDontBlock = (TickType_t)0;

    /* Create the first two tasks as described at the top of the file. */

    /* First create the structure used to pass parameters to the consumer tasks. */
    pxQueueParameters1 = (xBlockingQueueParameters*)pvPortMalloc(sizeof(xBlockingQueueParameters));

    /* Create the queue used by the first two tasks to pass the incrementing number.
     * Pass a pointer to the queue in the parameter structure. */
    pxQueueParameters1->xQueue = xQueueCreate(uxQueueSize1, (UBaseType_t)sizeof(uint16_t));

    /* The consumer is created first so gets a block time as described above. */
    pxQueueParameters1->xBlockTime = xBlockTime;

    /* Pass in the variable that this task is going to increment so we can check it
     * is still running. */
    pxQueueParameters1->psCheckVariable = &(sBlockingConsumerCount[0]);

    /* Create the structure used to pass parameters to the producer task. */
    pxQueueParameters2 = (xBlockingQueueParameters*)pvPortMalloc(sizeof(xBlockingQueueParameters));

    /* Pass the queue to this task also, using the parameter structure. */
    pxQueueParameters2->xQueue = pxQueueParameters1->xQueue;

    /* The producer is not going to block - as soon as it posts the consumer will
     * wake and remove the item so the producer should always have room to post. */
    pxQueueParameters2->xBlockTime = xDontBlock;

    /* Pass in the variable that this task is going to increment so we can check
     * it is still running. */
    pxQueueParameters2->psCheckVariable = &(sBlockingProducerCount[0]);

    pxQueueParameters1->paramNumber = 1;
    pxQueueParameters2->paramNumber = 1;


    /* Note the producer has a lower priority than the consumer when the tasks are
     * spawned. */
    // xTaskCreate(vBlockingQueueConsumer, "QConsB1", blckqSTACK_SIZE, (void*)pxQueueParameters1, uxPriority, &xTaskQConsB1);
    // xTaskCreate(vBlockingQueueProducer, "QProdB2", blckqSTACK_SIZE, (void*)pxQueueParameters2, tskIDLE_PRIORITY, &xTaskQProdB2);

    xTaskCreate(vBlockingQueueConsumer, "QConsB1", blckqSTACK_SIZE, (void*)pxQueueParameters1, 4, &xTaskQConsB1);
    xTaskCreate(vBlockingQueueProducer, "QProdB2", blckqSTACK_SIZE, (void*)pxQueueParameters2, 2, &xTaskQProdB2);



    /* Create the second two tasks as described at the top of the file.   This uses
     * the same mechanism but reverses the task priorities. */

    pxQueueParameters3 = (xBlockingQueueParameters*)pvPortMalloc(sizeof(xBlockingQueueParameters));
    pxQueueParameters3->xQueue = xQueueCreate(uxQueueSize1, (UBaseType_t)sizeof(uint16_t));
    pxQueueParameters3->xBlockTime = xDontBlock;
    pxQueueParameters3->psCheckVariable = &(sBlockingProducerCount[1]);

    pxQueueParameters4 = (xBlockingQueueParameters*)pvPortMalloc(sizeof(xBlockingQueueParameters));
    pxQueueParameters4->xQueue = pxQueueParameters3->xQueue;
    pxQueueParameters4->xBlockTime = xBlockTime;
    pxQueueParameters4->psCheckVariable = &(sBlockingConsumerCount[1]);

    pxQueueParameters3->paramNumber = 2;
    pxQueueParameters4->paramNumber = 2;


    // xTaskCreate(vBlockingQueueConsumer, "QConsB3", blckqSTACK_SIZE, (void*)pxQueueParameters3, tskIDLE_PRIORITY, &xTaskQConsB3);
    // xTaskCreate(vBlockingQueueProducer, "QProdB4", blckqSTACK_SIZE, (void*)pxQueueParameters4, uxPriority, &xTaskQProdB4);

    xTaskCreate(vBlockingQueueConsumer, "QConsB3", blckqSTACK_SIZE, (void*)pxQueueParameters3, 1, &xTaskQConsB3);
    xTaskCreate(vBlockingQueueProducer, "QProdB4", blckqSTACK_SIZE, (void*)pxQueueParameters4, 3, &xTaskQProdB4);


    /* Create the last two tasks as described above.  The mechanism is again just
     * the same.  This time both parameter structures are given a block time. */
    pxQueueParameters5 = (xBlockingQueueParameters*)pvPortMalloc(sizeof(xBlockingQueueParameters));
    pxQueueParameters5->xQueue = xQueueCreate(uxQueueSize5, (UBaseType_t)sizeof(uint16_t));
    pxQueueParameters5->xBlockTime = xBlockTime;
    pxQueueParameters5->psCheckVariable = &(sBlockingProducerCount[2]);

    pxQueueParameters6 = (xBlockingQueueParameters*)pvPortMalloc(sizeof(xBlockingQueueParameters));
    pxQueueParameters6->xQueue = pxQueueParameters5->xQueue;
    pxQueueParameters6->xBlockTime = xBlockTime;
    pxQueueParameters6->psCheckVariable = &(sBlockingConsumerCount[2]);

    pxQueueParameters5->paramNumber = 3;
    pxQueueParameters6->paramNumber = 3;


    // xTaskCreate(vBlockingQueueProducer, "QProdB5", blckqSTACK_SIZE, (void*)pxQueueParameters5, tskIDLE_PRIORITY, &xTaskQProdB5);
    // xTaskCreate(vBlockingQueueConsumer, "QConsB6", blckqSTACK_SIZE, (void*)pxQueueParameters6, tskIDLE_PRIORITY, &xTaskQConsB6);


    /* log the task handles */
    log_struct("BlockQ_TaskQConsB1", TYPE_TASK_HANDLE, xTaskQConsB1);
    log_struct("BlockQ_TaskQProdB2", TYPE_TASK_HANDLE, xTaskQProdB2);
    log_struct("BlockQ_TaskQConsB3", TYPE_TASK_HANDLE, xTaskQConsB3);
    log_struct("BlockQ_TaskQProdB4", TYPE_TASK_HANDLE, xTaskQProdB4);
    // log_struct("BlockQ_TaskQProdB5", TYPE_TASK_HANDLE, xTaskQProdB5);
    // log_struct("BlockQ_TaskQConsB6", TYPE_TASK_HANDLE, xTaskQConsB6);

    /* log the queue handles */
    log_struct("BlockQ_Queue1-2", TYPE_QUEUE_HANDLE, pxQueueParameters1->xQueue);
    log_struct("BlockQ_Queue3-4", TYPE_QUEUE_HANDLE, pxQueueParameters3->xQueue);
    // log_struct("BlockQ_Queue5-6", TYPE_QUEUE_HANDLE, pxQueueParameters5->xQueue);
}
/*-----------------------------------------------------------*/

static portTASK_FUNCTION(vBlockingQueueProducer, pvParameters)
{
    uint16_t usValue = 0;
    xBlockingQueueParameters* pxQueueParameters;
    short sErrorEverOccurred = pdFALSE;


    pxQueueParameters = (xBlockingQueueParameters*)pvParameters;

    uint8_t currentProducerNumber = pxQueueParameters->paramNumber;

    for (; ; )
    {
        if (xQueueSend(pxQueueParameters->xQueue, (void*)&usValue, pxQueueParameters->xBlockTime) != pdPASS)
        {
            sErrorEverOccurred = pdTRUE;
            console_print("BlockQ - ERROR: Producer %d failed to send value %d\n", currentProducerNumber, usValue);
        }
        else
        {
            /* We have successfully posted a message, so increment the variable
             * used to check we are still running. */
            if (sErrorEverOccurred == pdFALSE)
            {
                (*pxQueueParameters->psCheckVariable)++;
            }

            /* Increment the variable we are going to post next time round.  The
             * consumer will expect the numbers to	follow in numerical order. */
            ++usValue;

            console_print("BlockQ - Producer %d successfully sent value %d. Next value to send: %d\n", currentProducerNumber, usValue - 1, usValue);

#if configUSE_PREEMPTION == 0
            taskYIELD();
#endif

            if (usValue == limitValue) {
                console_print("BlockQ - Producer %d stopped\n", currentProducerNumber);
                sBlockingTasksPAlive[currentProducerNumber - 1] = 0;
                vTaskDelete(NULL);
            }
        }
    }
}
/*-----------------------------------------------------------*/

static portTASK_FUNCTION(vBlockingQueueConsumer, pvParameters)
{
    uint16_t usData, usExpectedValue = 0;
    xBlockingQueueParameters* pxQueueParameters;
    short sErrorEverOccurred = pdFALSE;


    pxQueueParameters = (xBlockingQueueParameters*)pvParameters;

    uint8_t currentConsumerNumber = pxQueueParameters->paramNumber;

    for (; ; )
    {
        if (xQueueReceive(pxQueueParameters->xQueue, &usData, pxQueueParameters->xBlockTime) == pdPASS)
        {
            if (usData != usExpectedValue)
            {
                /* Catch-up. */
                usExpectedValue = usData;

                sErrorEverOccurred = pdTRUE;

                console_print("BlockQ - ERROR: Consumer %d received value %d intead of value %d\n", currentConsumerNumber, usData, usExpectedValue);
            }
            else
            {
                /* We have successfully received a message, so increment the
                 * variable used to check we are still running. */
                if (sErrorEverOccurred == pdFALSE)
                {
                    (*pxQueueParameters->psCheckVariable)++;
                }

                /* Increment the value we expect to remove from the queue next time
                 * round. */
                ++usExpectedValue;

                console_print("BlockQ - Consumer %d successfully received value %d. Next value to receive: %d\n", currentConsumerNumber, usExpectedValue - 1, usExpectedValue);
            }

#if configUSE_PREEMPTION == 0
            {
                if (pxQueueParameters->xBlockTime == 0)
                {
                    taskYIELD();
                }
            }
#endif
            if (usExpectedValue == limitValue) {
                console_print("BlockQ - Consumer %d stopped\n", currentConsumerNumber);
                sBlockingTasksCAlive[currentConsumerNumber - 1] = 0;
                vTaskDelete(NULL);
            }
        }
    }
}
/*-----------------------------------------------------------*/

/* This is called to check that all the created tasks are still running. */
BaseType_t xAreBlockingQueuesStillRunning(void)
{
    static short sLastBlockingConsumerCount[blckqNUM_TASK_SETS] = { (uint16_t)0, (uint16_t)0, (uint16_t)0 };
    static short sLastBlockingProducerCount[blckqNUM_TASK_SETS] = { (uint16_t)0, (uint16_t)0, (uint16_t)0 };
    BaseType_t xReturn = pdPASS, xTasks;

    /* Not too worried about mutual exclusion on these variables as they are 16
     * bits and we are only reading them. We also only care to see if they have
     * changed or not.
     *
     * Loop through each check variable to and return pdFALSE if any are found not
     * to have changed since the last call. */

    for (xTasks = 0; xTasks < blckqNUM_TASK_SETS; xTasks++)
    {
        if (sBlockingConsumerCount[xTasks] == sLastBlockingConsumerCount[xTasks])
        {
            xReturn = pdFALSE;
        }

        sLastBlockingConsumerCount[xTasks] = sBlockingConsumerCount[xTasks];

        if (sBlockingProducerCount[xTasks] == sLastBlockingProducerCount[xTasks])
        {
            xReturn = pdFALSE;
        }

        sLastBlockingProducerCount[xTasks] = sBlockingProducerCount[xTasks];
    }

    return xReturn;
}

BaseType_t xAreBlockingQueuesAlive(void)
{
    return (sBlockingTasksCAlive[0] + sBlockingTasksCAlive[1] + sBlockingTasksCAlive[2] + sBlockingTasksPAlive[0] + sBlockingTasksPAlive[1] + sBlockingTasksPAlive[2]) != 0;
}