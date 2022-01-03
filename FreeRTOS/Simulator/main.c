/* Standard includes. */
/* Platform-independent. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Platform-dependent. */
#if defined _WIN32
    #include <conio.h>
#elif defined __unix__
    #include <unistd.h>
    #include <stdarg.h>
    #include <errno.h>
    #include <sys/select.h>
    #include <time.h>
#endif

/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include <queue.h>
#include <timers.h>
#include <semphr.h>

/* Standard demo includes. */
#include "BlockQ.h"
#include "integer.h"
#include "semtest.h"
#include "PollQ.h"
#include "GenQTest.h"
#include "QPeek.h"
#include "recmutex.h"
#include "flop.h"
#include "TimerDemo.h"
#include "countsem.h"
#include "death.h"
#include "dynamic.h"
#include "QueueSet.h"
#include "QueueOverwrite.h"
#include "EventGroupsDemo.h"
#include "IntSemTest.h"
#include "TaskNotify.h"
#include "QueueSetPolling.h"
#include "StaticAllocation.h"
#include "blocktim.h"
#include "AbortDelay.h"
#include "MessageBufferDemo.h"
#include "StreamBufferDemo.h"
#include "StreamBufferInterrupt.h"
#include "MessageBufferAMP.h"

/* Local includes. */
#include "console.h"
/*#include "structuresLog.h"*/

/* Priorities at which the tasks are created. */
#define mainCHECK_TASK_PRIORITY			( configMAX_PRIORITIES - 2 )
#define mainQUEUE_POLL_PRIORITY			( tskIDLE_PRIORITY + 1 )
#define mainSEM_TEST_PRIORITY			( tskIDLE_PRIORITY + 1 )
#define mainBLOCK_Q_PRIORITY			( tskIDLE_PRIORITY + 2 )
#define mainCREATOR_TASK_PRIORITY		( tskIDLE_PRIORITY + 3 )
#define mainFLASH_TASK_PRIORITY			( tskIDLE_PRIORITY + 1 )
#define mainINTEGER_TASK_PRIORITY		( tskIDLE_PRIORITY )
#define mainGEN_QUEUE_TASK_PRIORITY		( tskIDLE_PRIORITY )
#define mainFLOP_TASK_PRIORITY			( tskIDLE_PRIORITY )
#define mainQUEUE_OVERWRITE_PRIORITY	( tskIDLE_PRIORITY )

#define mainTIMER_TEST_PERIOD			( 50 )

/*-----------------------------------------------------------*/

/*
 * Portable sleep function
 */
void vSleepMS(const unsigned long ulMSToSleep);

/*
 * Prototypes for the standard FreeRTOS application hook (callback) functions
 * implemented within this file.  See http://www.freertos.org/a00016.html .
 */
void vApplicationMallocFailedHook( void );
void vApplicationIdleHook( void );
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName );
void vApplicationTickHook( void );
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize );

/*-----------------------------------------------------------*/

/* Custom Task function prototypes. */
static void prvCheckTask( void *pvParameters );

/* A task that is created from the idle task to test the functionality of
eTaskStateGet(). */
static void prvTestTask( void *pvParameters );

/*
 * Called from the idle task hook function to demonstrate a few utility
 * functions that are not demonstrated by any of the standard demo tasks.
 */
static void prvDemonstrateTaskStateAndHandleGetFunctions( void );

/*
 * Called from the idle task hook function to demonstrate the use of
 * xTimerPendFunctionCall() as xTimerPendFunctionCall() is not demonstrated by
 * any of the standard demo tasks.
 */
static void prvDemonstratePendingFunctionCall( void );

/*
 * The function that is pended by prvDemonstratePendingFunctionCall().
 */
static void prvPendedFunction( void *pvParameter1, uint32_t ulParameter2 );

/*
 * prvDemonstrateTimerQueryFunctions() is called from the idle task hook
 * function to demonstrate the use of functions that query information about a
 * software timer.  prvTestTimerCallback() is the callback function for the
 * timer being queried.
 */
static void prvDemonstrateTimerQueryFunctions( void );
static void prvTestTimerCallback( TimerHandle_t xTimer );

/*
 * A task to demonstrate the use of the xQueueSpacesAvailable() function.
 */
static void prvDemoQueueSpaceFunctions( void *pvParameters );

/*
 * Tasks that ensure indefinite delays are truly indefinite.
 */
static void prvPermanentlyBlockingSemaphoreTask( void *pvParameters );
static void prvPermanentlyBlockingNotificationTask( void *pvParameters );

/*-----------------------------------------------------------*/

/* The variable into which error messages are latched. */
static char const *pcStatusMessage = "No errors";
int xErrorCount = 0;

/* This semaphore is created purely to test using the vSemaphoreDelete(). It has no other purpose. */
static SemaphoreHandle_t xMutexToDelete = NULL;

/* When configSUPPORT_STATIC_ALLOCATION is set to 1 the application writer can
use a callback function to optionally provide the memory required by the idle
and timer tasks.  This is the stack that will be used by the timer task.  It is
declared here, as a global, so it can be checked by a test that is implemented
in a different file. */
StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];

/* TaskHandle for the tasks created in main.c */
TaskHandle_t xTaskCheck;
TaskHandle_t xTaskQSpace;
TaskHandle_t xTaskBlockSem;
TaskHandle_t xTaskBlockNoti;
/*-----------------------------------------------------------*/

int main( void )
{
    printf("Size of TaskHandle: %d\n", sizeof(StaticTask_t));
    printf("Size of QueueHandle: %d\n", sizeof(StaticQueue_t));
    printf("Size of TimerHandle: %d\n", sizeof(StaticTimer_t));
    printf("Size of SemaphoreHandle: %d\n", sizeof(StaticSemaphore_t));
    printf("Size of CountingSemaphore: %d\n", sizeof(StaticQueue_t));
    printf("Size of EventGroupHandle: %d\n", sizeof(StaticEventGroup_t));
    printf("Size of StreamBufferHandle: %d\n", sizeof(StaticStreamBuffer_t));
    printf("Size of MessageBufferHandle: %d\n", sizeof(StaticMessageBuffer_t));
    printf("Size of QueueSetHandle: %d\n", sizeof(StaticQueue_t));
    printf("Size of the Static Stack: %d\n", configMINIMAL_STACK_SIZE * 2);
    printf("Size of a Queue Registry Item: %d\n", queue_registry_item_size());

    console_init();
    log_start();

#if defined TASK_CHECK
    /* Start the check task as described at the top of this file. */
    xTaskCreate( prvCheckTask, "Check", configMINIMAL_STACK_SIZE, NULL, mainCHECK_TASK_PRIORITY, &xTaskCheck );

    /* log the "Check" task handle */
    log_struct("CheckTask", TYPE_TASK_HANDLE, xTaskCheck);
#endif

    /* Create the standard demo tasks. */
#if defined TASK_TASK_NOTIFY
    vStartTaskNotifyTask();
#endif
#if defined TASK_BLOCKING_QUEUE
    vStartBlockingQueueTasks( mainBLOCK_Q_PRIORITY );
#endif
#if defined TASK_SEM_TEST
    vStartSemaphoreTasks( mainSEM_TEST_PRIORITY );
#endif
#if defined TASK_POLL_QUEUE
    vStartPolledQueueTasks( mainQUEUE_POLL_PRIORITY );
#endif
#if defined TASK_INTEGER
    vStartIntegerMathTasks( mainINTEGER_TASK_PRIORITY );
#endif
#if defined TASK_GEN_QUEUE
    vStartGenericQueueTasks( mainGEN_QUEUE_TASK_PRIORITY );
#endif
#if defined TASK_PEEK_QUEUE
    vStartQueuePeekTasks();
#endif
#if defined TASK_FLOP
    vStartMathTasks( mainFLOP_TASK_PRIORITY );
#endif
#if defined TASK_REC_MUTEX
    vStartRecursiveMutexTasks();
#endif
#if defined TASK_COUNT_SEM
    vStartCountingSemaphoreTasks();
#endif
#if defined TASK_DYNAMIC_PRIOR
    vStartDynamicPriorityTasks();
#endif
#if defined TASK_QUEUE_OVERWRITE
    vStartQueueOverwriteTask( mainQUEUE_OVERWRITE_PRIORITY );
#endif
#if defined TASK_EVENT_GROUPS
    vStartEventGroupTasks();
#endif
#if defined TASK_INT_SEM_TEST
    vStartInterruptSemaphoreTasks();
#endif
#if defined TASK_BLOCK_TIM
    vCreateBlockTimeTasks();
#endif
#if defined TASK_ABORT_DELAY
    vCreateAbortDelayTasks();
#endif
#if defined TASK_QUEUE_SPACE_AVAIL
    xTaskCreate( prvDemoQueueSpaceFunctions, "QSpace", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, &xTaskQSpace );

    /* log the "QSpace" task handle */
    log_struct("QSpaceTask", TYPE_TASK_HANDLE, xTaskQSpace);
#endif
#if defined TASK_INDEF_DELAY_SEM
    xTaskCreate( prvPermanentlyBlockingSemaphoreTask, "BlockSem", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, &xTaskBlockSem );

    /* log the "BlockSem" task handle */
    log_struct("BlockSemTask", TYPE_TASK_HANDLE, xTaskBlockSem);
#endif
#if defined TASK_INDEF_DELAY_NOTIF
    xTaskCreate( prvPermanentlyBlockingNotificationTask, "BlockNoti", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, &xTaskBlockNoti );

    /* log the "BlockNoti" task handle */
    log_struct("BlockNotiTask", TYPE_TASK_HANDLE, xTaskBlockNoti);
#endif

#if defined TASK_MESSAGE_BUFFER
    vStartMessageBufferTasks( configMINIMAL_STACK_SIZE );
#endif
#if defined TASK_STREAM_BUFFER
    vStartStreamBufferTasks();
#endif
#if defined TASK_STREAM_BUFF_INT
    vStartStreamBufferInterruptDemo();
#endif
#if defined TASK_MESSAGE_BUFF_AMP
    vStartMessageBufferAMPTasks( configMINIMAL_STACK_SIZE );
#endif

    #if ( configUSE_QUEUE_SETS == 1 )
        {
            #if defined TASK_QUEUE_SET
                vStartQueueSetTasks();
            #endif
            #if defined TASK_QUEUE_SET_POLL
                vStartQueueSetPollingTask();
            #endif
        }
    #endif

    #if ( configSUPPORT_STATIC_ALLOCATION == 1 )
        {
            #if defined TASK_STATIC_ALLOC
                vStartStaticallyAllocatedTasks();
            #endif
        }
    #endif

    #if ( configUSE_PREEMPTION != 0 )
        {
            #if defined TASK_TIMER
                /* Don't expect these tasks to pass when preemption is not used. */
                vStartTimerDemoTask( mainTIMER_TEST_PERIOD );
            #endif          
        }
    #endif
    
#if defined TASK_DEATH
    /* The suicide tasks must be created last as they need to know how many
     * tasks were running prior to their creation.  This then allows them to
     * ascertain whether or not the correct/expected number of tasks are running at
     * any given time. */
    vCreateSuicidalTasks( mainCREATOR_TASK_PRIORITY );
#endif

#if defined TEST_DELETE_MUTEX
    /* Create the semaphore that will be deleted in the idle task hook.  This
     * is done purely to test the use of vSemaphoreDelete(). */
    xMutexToDelete = xSemaphoreCreateMutex();

    /* log the "xMutexToDelete" semaphore handle */
    log_struct("MutexToDelete", TYPE_SEMAPHORE_HANDLE, xMutexToDelete);
#endif

    /* log the OS structures that were creatred during the creation of the tasks */
    log_queue_registry();
    log_timers_struct();
    log_tasks_struct();

    log_end();

    /* Start the scheduler itself. */
    vTaskStartScheduler();

    /* Should never get here unless there was not enough heap space to create
     * the idle and other system tasks. */
    return 0;
}
/*-----------------------------------------------------------*/

void vSleepMS(const unsigned long ulMSToSleep) {
    #ifdef __unix__
        struct timespec ts;
        ts.tv_sec = ulMSToSleep / 1000;
        ts.tv_nsec = ulMSToSleep % 1000l * 1000000l;
    #endif

    #if defined _WIN32
        Sleep( ulMSToSleep );
    #elif defined __unix__
        nanosleep( &ts, NULL );
    #endif
}
/*-----------------------------------------------------------*/

void vAssertCalled( unsigned long ulLine, const char * const pcFileName )
{
    static BaseType_t xPrinted = pdFALSE;
    volatile uint32_t ulSetToNonZeroInDebuggerToContinue = 0;

    /* Called if an assertion passed to configASSERT() fails.  See
    http://www.freertos.org/a00110.html#configASSERT for more information. */

    /* Parameters are not used. */
    ( void ) ulLine;
    ( void ) pcFileName;

    printf( "ASSERT! Line %ld, file %s\n", ulLine, pcFileName );

    taskENTER_CRITICAL();
    {
        #ifdef WIN32
            /* Cause debugger break point if being debugged. */
            __debugbreak();
        #endif

        /* You can step out of this function to debug the assertion by using
        the debugger to set ulSetToNonZeroInDebuggerToContinue to a non-zero
        value. */
        while( ulSetToNonZeroInDebuggerToContinue == 0 )
        {
            #if defined _WIN32
                __asm { NOP };
                __asm { NOP };
            #elif defined __unix__
                __asm("nop");
                __asm("nop");
            #endif
        }
    }
    taskEXIT_CRITICAL();
}
/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook( void )
{
    /* vApplicationMallocFailedHook() will only be called if
    configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
    function that will get called if a call to pvPortMalloc() fails.
    pvPortMalloc() is called internally by the kernel whenever a task, queue,
    timer or semaphore is created.  It is also called by various parts of the
    demo application.  If heap_1.c, heap_2.c or heap_4.c is being used, then the
    size of the	heap available to pvPortMalloc() is defined by
    configTOTAL_HEAP_SIZE in FreeRTOSConfig.h, and the xPortGetFreeHeapSize()
    API function can be used to query the size of free heap space that remains
    (although it does not provide information on how the remaining heap might be
    fragmented).  See http://www.freertos.org/a00111.html for more
    information. */
    vAssertCalled( __LINE__, __FILE__ );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
    /* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
    to 1 in FreeRTOSConfig.h.  It will be called on each iteration of the idle
    task.  It is essential that code added to this hook function never attempts
    to block in any way (for example, call xQueueReceive() with a block time
    specified, or call vTaskDelay()).  If application tasks make use of the
    vTaskDelete() API function to delete themselves then it is also important
    that vApplicationIdleHook() is permitted to return to its calling function,
    because it is the responsibility of the idle task to clean up memory
    allocated by the kernel to any task that has since deleted itself. */

const unsigned long ulMSToSleep = 15;
void *pvAllocated;

    /* Sleep to reduce CPU load, but don't sleep indefinitely in case there are
    tasks waiting to be terminated by the idle task. */
    vSleepMS( ulMSToSleep );

#if defined TASK_TASK_INTERNAL
    /* Demonstrate a few utility functions that are not demonstrated by any of
    the standard demo tasks. */
    prvDemonstrateTaskStateAndHandleGetFunctions();
#endif

#if defined TASK_PEND_FUNC_CALL
    /* Demonstrate the use of xTimerPendFunctionCall(), which is not
    demonstrated by any of the standard demo tasks. */
    prvDemonstratePendingFunctionCall();
#endif

#if defined TASK_TIMER_QUERY
    /* Demonstrate the use of functions that query information about a software
    timer. */
    prvDemonstrateTimerQueryFunctions();
#endif

#if defined TEST_DELETE_MUTEX
    /* If xMutexToDelete has not already been deleted, then delete it now.
    This is done purely to demonstrate the use of, and test, the
    vSemaphoreDelete() macro.  Care must be taken not to delete a semaphore
    that has tasks blocked on it. */
    if( xMutexToDelete != NULL )
    {
        /* For test purposes, add the mutex to the registry, then remove it
        again, before it is deleted - checking its name is as expected before
        and after the assertion into the registry and its removal from the
        registry. */
        configASSERT( pcQueueGetName( xMutexToDelete ) == NULL );
        vQueueAddToRegistry( xMutexToDelete, "Test_Mutex" );
        configASSERT( strcmp( pcQueueGetName( xMutexToDelete ), "Test_Mutex" ) == 0 );
        vQueueUnregisterQueue( xMutexToDelete );
        configASSERT( pcQueueGetName( xMutexToDelete ) == NULL );

        vSemaphoreDelete( xMutexToDelete );
        xMutexToDelete = NULL;
    }
#endif

    /* Exercise heap_5 a bit.  The malloc failed hook will trap failed
    allocations so there is no need to test here. */
    pvAllocated = pvPortMalloc( ( rand() % 500 ) + 1 );
    vPortFree( pvAllocated );
}
/*-----------------------------------------------------------*/

/* Called by vApplicationTickHook(), which is defined in main.c. */
void vApplicationTickHook( void )
{
TaskHandle_t xTimerTask;
    
#if defined TIMER_PERIODIC_ISR_TESTS
    /* Call the periodic timer test, which tests the timer API functions that
    can be called from an ISR. */
    #if( configUSE_PREEMPTION != 0 )
    {
		/* Only created when preemption is used. */
		vTimerPeriodicISRTests();
	}
    #endif
#endif

#if defined QUEUE_OVERWRITE_PERIODIC_ISR
    /* Call the periodic queue overwrite from ISR demo. */
    vQueueOverwritePeriodicISRDemo();
#endif

    #if( configUSE_QUEUE_SETS == 1 ) /* Remove the tests if queue sets are not defined. */
    {
		/* Write to a queue that is in use as part of the queue set demo to
		demonstrate using queue sets from an ISR. */
        #if defined QUEUE_SET_ACCESS_ISR
		    vQueueSetAccessQueueSetFromISR();
        #endif
        #if defined QUEUE_SET_ACCESS_POLL_ISR
		    vQueueSetPollingInterruptAccess();
        #endif
	}
    #endif
    
#if defined EVENT_GROUP_ISR
    /* Exercise event groups from interrupts. */
    vPeriodicEventGroupsProcessing();
#endif

#if defined SEM_TEST_ISR
    /* Exercise giving mutexes from an interrupt. */
    vInterruptSemaphorePeriodicTest();
#endif
    
#if defined NOTIFY_TASK_ISR
    /* Exercise using task notifications from an interrupt. */
    xNotifyTaskFromISR();
#endif

#if defined STREAM_BUFFER_PROC
    /* Writes to stream buffer byte by byte to test the stream buffer trigger
    level functionality. */
    //vPeriodicStreamBufferProcessing();
#endif

#if defined STREAM_BUFFER_SEND_ISR
    /* Writes a string to a string buffer four bytes at a time to demonstrate
    a stream being sent from an interrupt to a task. */
    vBasicStreamBufferSendFromISR();
#endif

    /* For code coverage purposes. */
    xTimerTask = xTimerGetTimerDaemonTaskHandle();
    configASSERT( uxTaskPriorityGetFromISR( xTimerTask ) == configTIMER_TASK_PRIORITY );
}
/*-----------------------------------------------------------*/

/* configUSE_STATIC_ALLOCATION is set to 1, so the application must provide an
implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
used by the Idle task. */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
/* If the buffers to be provided to the Idle task are declared inside this
function then they must be declared static - otherwise they will be allocated on
the stack and so not exists after this function exits. */
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
    state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
/*-----------------------------------------------------------*/

/* configUSE_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
application must provide an implementation of vApplicationGetTimerTaskMemory()
to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize )
{
/* If the buffers to be provided to the Timer task are declared inside this
function then they must be declared static - otherwise they will be allocated on
the stack and so not exists after this function exits. */
    static StaticTask_t xTimerTaskTCB;

    /* Pass out a pointer to the StaticTask_t structure in which the Timer
    task's state will be stored. */
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

    /* Pass out the array that will be used as the Timer task's stack. */
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;

    /* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

/* Custom Task definitions */

static void prvCheckTask( void * pvParameters )
{
    TickType_t xNextWakeTime;
    const TickType_t xCycleFrequency = pdMS_TO_TICKS( 10000UL );
    //HeapStats_t xHeapStats;

    /* Just to remove compiler warning. */
    ( void ) pvParameters;

    /* Initialise xNextWakeTime - this only needs to be done once. */
    xNextWakeTime = xTaskGetTickCount();

    for( ; ; )
    {
        /* Place this task in the blocked state until it is time to run again. */
        vTaskDelayUntil( &xNextWakeTime, xCycleFrequency );

        /* Check the standard demo tasks are running without error. */
#if ( configUSE_PREEMPTION != 0 )
        {
                #if defined TASK_TIMER
                    /* These tasks are only created when preemption is used. */
                    if( xAreTimerDemoTasksStillRunning( xCycleFrequency ) != pdTRUE )
                    {
                        pcStatusMessage = "Error: TimerDemo";
                        xErrorCount++;
                    }
                #endif
            }
#endif

#if defined TASK_STREAM_BUFFER
        if(xAreStreamBuffersAlive() == pdTRUE && xAreStreamBufferTasksStillRunning() != pdTRUE )
        {
            pcStatusMessage = "Error:  StreamBuffer";
            xErrorCount++;
        }
#endif
#if defined TASK_MESSAGE_BUFFER
        if( xAreMessageBuffersAlive() == pdTRUE && xAreMessageBufferTasksStillRunning() != pdTRUE )
        {
            pcStatusMessage = "Error:  MessageBuffer";
            xErrorCount++;
        }
#endif
#if defined TASK_TASK_NOTIFY
        if( xAreTaskNotificationTasksStillRunning() != pdTRUE )
        {
            pcStatusMessage = "Error:  Notification";
            xErrorCount++;
        }
#endif

            /* else if( xAreTaskNotificationArrayTasksStillRunning() != pdTRUE )
             * {
             * pcStatusMessage = "Error:  NotificationArray";
             *  xErrorCount++;
             * } */
#if defined TASK_INT_SEM_TEST
        if( xAreInterruptSemaphoreTasksStillRunning() != pdTRUE )
        {
            pcStatusMessage = "Error: IntSem";
            xErrorCount++;
        }
#endif
#if defined TASK_EVENT_GROUPS
        if( xAreEventGroupTasksStillRunning() != pdTRUE )
        {
            pcStatusMessage = "Error: EventGroup";
            xErrorCount++;
        }
#endif
#if defined TASK_INTEGER
        if( xAreIntegerMathsTaskStillRunning() != pdTRUE )
        {
            pcStatusMessage = "Error: IntMath";
            xErrorCount++;
        }
#endif
#if defined TASK_GEN_QUEUE
        if( xAreGenericQueueTasksStillRunning() != pdTRUE )
        {
            pcStatusMessage = "Error: GenQueue";
            xErrorCount++;
        }
#endif
#if defined TASK_PEEK_QUEUE
        if( xAreQueuePeekTasksStillRunning() != pdTRUE )
        {
            pcStatusMessage = "Error: QueuePeek";
            xErrorCount++;
        }
#endif
#if defined TASK_BLOCKING_QUEUE
        if( xAreBlockingQueuesAlive() == pdTRUE && xAreBlockingQueuesStillRunning() != pdTRUE )
        {
            pcStatusMessage = "Error: BlockQueue";
            xErrorCount++;
        }
#endif
#if defined TASK_SEM_TEST
        if( xAreSemaphoresAlive() == pdTRUE && xAreSemaphoreTasksStillRunning() != pdTRUE )
        {
            pcStatusMessage = "Error: SemTest";
            xErrorCount++;
        }
#endif
#if defined TASK_POLL_QUEUE
        if( xArePollingQueuesAlive() == pdTRUE && xArePollingQueuesStillRunning() != pdTRUE )
        {
            pcStatusMessage = "Error: PollQueue";
            xErrorCount++;
        }
#endif
#if defined TASK_FLOP
        if( xAreMathsTaskStillRunning() != pdPASS )
        {
            pcStatusMessage = "Error: Flop";
            xErrorCount++;
        }
#endif
#if defined TASK_REC_MUTEX
        if( xAreRecursiveMutexTasksStillRunning() != pdTRUE )
        {
            pcStatusMessage = "Error: RecMutex";
            xErrorCount++;
        }
#endif
#if defined TASK_COUNT_SEM
        if(xAreCountSemAlive() == pdTRUE && xAreCountingSemaphoreTasksStillRunning() != pdTRUE )
        {
            pcStatusMessage = "Error: CountSem";
            xErrorCount++;
        }
#endif
#if defined TASK_DEATH
        if( xIsCreateTaskStillRunning() != pdTRUE )
        {
            pcStatusMessage = "Error: Death";
            xErrorCount++;
        }
#endif
#if defined TASK_DYNAMIC_PRIOR
        if( xAreDynamicPriorityTasksStillRunning() != pdPASS )
        {
            pcStatusMessage = "Error: Dynamic";
            xErrorCount++;
        }
#endif
#if defined TASK_QUEUE_OVERWRITE
        if( xIsQueueOverwriteTaskStillRunning() != pdPASS )
        {
            pcStatusMessage = "Error: Queue overwrite";
            xErrorCount++;
        }
#endif
#if defined TASK_BLOCK_TIM
        if( xAreBlockTimeTestTasksStillRunning() != pdPASS )
        {
            pcStatusMessage = "Error: Block time";
            xErrorCount++;
        }
#endif
#if defined TASK_ABORT_DELAY
        if( xAreAbortDelayTestTasksStillRunning() != pdPASS )
        {
            pcStatusMessage = "Error: Abort delay";
            xErrorCount++;
        }
#endif
#if defined STREM_BUFFER_SEND_ISR
        if( xIsInterruptStreamBufferDemoStillRunning() != pdPASS )
        {
            pcStatusMessage = "Error: Stream buffer interrupt";
            xErrorCount++;
        }
#endif
#if defined TASK_MESSAGE_BUFF_AMP
        if( xAreMessageBufferAMPTasksStillRunning() != pdPASS )
        {
            pcStatusMessage = "Error: Message buffer AMP";
            xErrorCount++;
        }
#endif

#if ( configUSE_QUEUE_SETS == 1 )
        #if defined TASK_QUEUE_SET
            if( xAreQueueSetTasksStillRunning() != pdPASS )
            {
                pcStatusMessage = "Error: Queue set";
                xErrorCount++;
            }
        #endif
        #if defined TASK_QUEUE_SET_POLL
            if( xAreQueueSetPollTasksStillRunning() != pdPASS )
            {
                pcStatusMessage = "Error: Queue set polling";
                xErrorCount++;
            }
        #endif
#endif /* if ( configUSE_QUEUE_SETS == 1 ) */

#if ( configSUPPORT_STATIC_ALLOCATION == 1 )
    #if defined TASK_STATIC_ALLOC
            if( xAreStaticAllocationTasksStillRunning() != pdPASS )
            {
                xErrorCount++;
                pcStatusMessage = "Error: Static allocation";
            }
    #endif
#endif /* configSUPPORT_STATIC_ALLOCATION */

        printf( "%s - tick count %zu \r\n",
                pcStatusMessage,
                xTaskGetTickCount());
        /*
        if( xErrorCount != 0 )
        {
            exit( 1 );
        }
        */

        /* Reset the error condition */
        pcStatusMessage = "OK: No errors";
    }
}

/*-----------------------------------------------------------*/

static void prvTestTask( void *pvParameters )
{
    const unsigned long ulMSToSleep = 5;

    /* Just to remove compiler warnings. */
    ( void ) pvParameters;

    /* This task is just used to test the eTaskStateGet() function.  It
    does not have anything to do. */
    for( ;; )
    {
        /* Sleep to reduce CPU load, but don't sleep indefinitely in case there are
        tasks waiting to be terminated by the idle task. */
        vSleepMS( ulMSToSleep );
    }
}
/*-----------------------------------------------------------*/

static void prvPendedFunction( void *pvParameter1, uint32_t ulParameter2 )
{
    static uint32_t ulLastParameter1 = 1000UL, ulLastParameter2 = 0UL;
    uint32_t ulParameter1;

    ulParameter1 = ( uint32_t ) pvParameter1;

    /* Ensure the parameters are as expected. */
    configASSERT( ulParameter1 == ( ulLastParameter1 + 1 ) );
    configASSERT( ulParameter2 == ( ulLastParameter2 + 1 ) );

    /* Remember the parameters for the next time the function is called. */
    ulLastParameter1 = ulParameter1;
    ulLastParameter2 = ulParameter2;

    /* Remove compiler warnings in case configASSERT() is not defined. */
    ( void ) ulLastParameter1;
    ( void ) ulLastParameter2;
}
/*-----------------------------------------------------------*/

static void prvTestTimerCallback( TimerHandle_t xTimer )
{
    /* This is the callback function for the timer accessed by
    prvDemonstrateTimerQueryFunctions().  The callback does not do anything. */
    ( void ) xTimer;
}
/*-----------------------------------------------------------*/

static void prvDemonstrateTimerQueryFunctions( void )
{
    static TimerHandle_t xTimer = NULL;
    const char *pcTimerName = "TestTimer";
    volatile TickType_t xExpiryTime;
    const TickType_t xDontBlock = 0;

    if( xTimer == NULL )
    {
        xTimer = xTimerCreate( pcTimerName, portMAX_DELAY, pdTRUE, NULL, prvTestTimerCallback );

        if( xTimer != NULL )
        {
            /* Called from the idle task so a block time must not be
            specified. */
            xTimerStart( xTimer, xDontBlock );
        }
    }

    if( xTimer != NULL )
    {
        /* Demonstrate querying a timer's name. */
        configASSERT( strcmp( pcTimerGetName( xTimer ), pcTimerName ) == 0 );

        /* Demonstrate querying a timer's period. */
        configASSERT( xTimerGetPeriod( xTimer ) == portMAX_DELAY );

        /* Demonstrate querying a timer's next expiry time, although nothing is
        done with the returned value.  Note if the expiry time is less than the
        maximum tick count then the expiry time has overflowed from the current
        time.  In this case the expiry time was set to portMAX_DELAY, so it is
        expected to be less than the current time until the current time has
        itself overflowed. */
        xExpiryTime = xTimerGetExpiryTime( xTimer );
        ( void ) xExpiryTime;
    }
}
/*-----------------------------------------------------------*/

static void prvDemonstratePendingFunctionCall( void )
{
    static uint32_t ulParameter1 = 1000UL, ulParameter2 = 0UL;
    const TickType_t xDontBlock = 0; /* This is called from the idle task so must *not* attempt to block. */

    /* prvPendedFunction() just expects the parameters to be incremented by one
    each time it is called. */
    ulParameter1++;
    ulParameter2++;

    /* Pend the function call, sending the parameters. */
    xTimerPendFunctionCall( prvPendedFunction, ( void * ) ulParameter1, ulParameter2, xDontBlock );
}
/*-----------------------------------------------------------*/

static void prvDemonstrateTaskStateAndHandleGetFunctions( void )
{
    TaskHandle_t xIdleTaskHandle, xTimerTaskHandle;
    char * pcTaskName;
    static portBASE_TYPE xPerformedOneShotTests = pdFALSE;
    TaskHandle_t xTestTask;
    TaskStatus_t xTaskInfo;
    extern StackType_t uxTimerTaskStack[];

    /* Demonstrate the use of the xTimerGetTimerDaemonTaskHandle() and
     * xTaskGetIdleTaskHandle() functions.  Also try using the function that sets
     * the task number. */
    xIdleTaskHandle = xTaskGetIdleTaskHandle();
    xTimerTaskHandle = xTimerGetTimerDaemonTaskHandle();

    /* This is the idle hook, so the current task handle should equal the
     * returned idle task handle. */
    if( xTaskGetCurrentTaskHandle() != xIdleTaskHandle )
    {
        pcStatusMessage = "Error:  Returned idle task handle was incorrect";
        xErrorCount++;
    }

    /* Check the same handle is obtained using the idle task's name.  First try
     * with the wrong name, then the right name. */
    if( xTaskGetHandle( "Idle" ) == xIdleTaskHandle )
    {
        pcStatusMessage = "Error:  Returned handle for name Idle was incorrect";
        xErrorCount++;
    }

    if( xTaskGetHandle( "IDLE" ) != xIdleTaskHandle )
    {
        pcStatusMessage = "Error:  Returned handle for name Idle was incorrect";
        xErrorCount++;
    }

    /* Check the timer task handle was returned correctly. */
    pcTaskName = pcTaskGetName( xTimerTaskHandle );

    if( strcmp( pcTaskName, "Tmr Svc" ) != 0 )
    {
        pcStatusMessage = "Error:  Returned timer task handle was incorrect";
        xErrorCount++;
    }

    if( xTaskGetHandle( "Tmr Svc" ) != xTimerTaskHandle )
    {
        pcStatusMessage = "Error:  Returned handle for name Tmr Svc was incorrect";
        xErrorCount++;
    }

    /* This task is running, make sure it's state is returned as running. */
    if( eTaskStateGet( xIdleTaskHandle ) != eRunning )
    {
        pcStatusMessage = "Error:  Returned idle task state was incorrect";
        xErrorCount++;
    }

    /* If this task is running, then the timer task must be blocked. */
    if( eTaskStateGet( xTimerTaskHandle ) != eBlocked )
    {
        pcStatusMessage = "Error:  Returned timer task state was incorrect";
        xErrorCount++;
    }

    /* Also with the vTaskGetInfo() function. */
    vTaskGetInfo( xTimerTaskHandle, /* The task being queried. */
                  &xTaskInfo,       /* The structure into which information on the task will be written. */
                  pdTRUE,           /* Include the task's high watermark in the structure. */
                  eInvalid );       /* Include the task state in the structure. */

    /* Check the information returned by vTaskGetInfo() is as expected. */
    if( ( xTaskInfo.eCurrentState != eBlocked ) ||
        ( strcmp( xTaskInfo.pcTaskName, "Tmr Svc" ) != 0 ) ||
        ( xTaskInfo.uxCurrentPriority != configTIMER_TASK_PRIORITY ) ||
        ( xTaskInfo.pxStackBase != uxTimerTaskStack ) ||
        ( xTaskInfo.xHandle != xTimerTaskHandle ) )
    {
        pcStatusMessage = "Error:  vTaskGetInfo() returned incorrect information about the timer task";
        xErrorCount++;
    }

    /* Other tests that should only be performed once follow.  The test task
     * is not created on each iteration because to do so would cause the death
     * task to report an error (too many tasks running). */
    if( xPerformedOneShotTests == pdFALSE )
    {
        /* Don't run this part of the test again. */
        xPerformedOneShotTests = pdTRUE;

        /* Create a test task to use to test other eTaskStateGet() return values. */
        if( xTaskCreate( prvTestTask, "Test", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, &xTestTask ) == pdPASS )
        {
            /* If this task is running, the test task must be in the ready state. */
            if( eTaskStateGet( xTestTask ) != eReady )
            {
                pcStatusMessage = "Error: Returned test task state was incorrect 1";
                xErrorCount++;
            }

            /* Now suspend the test task and check its state is reported correctly. */
            vTaskSuspend( xTestTask );

            if( eTaskStateGet( xTestTask ) != eSuspended )
            {
                pcStatusMessage = "Error: Returned test task state was incorrect 2";
                xErrorCount++;
            }

            /* Now delete the task and check its state is reported correctly. */
            vTaskDelete( xTestTask );

            if( eTaskStateGet( xTestTask ) != eDeleted )
            {
                pcStatusMessage = "Error: Returned test task state was incorrect 3";
                xErrorCount++;
            }
        }
    }
}
/*-----------------------------------------------------------*/

static void prvDemoQueueSpaceFunctions( void *pvParameters )
{
    QueueHandle_t xQueue = NULL;
    const unsigned portBASE_TYPE uxQueueLength = 10;
    unsigned portBASE_TYPE uxReturn, x;

    /* Remove compiler warnings. */
    ( void ) pvParameters;

    /* Create the queue that will be used.  Nothing is actually going to be
    sent or received so the queue item size is set to 0. */
    xQueue = xQueueCreate( uxQueueLength, 0 );
    configASSERT( xQueue );

    for( ;; )
    {
        for( x = 0; x < uxQueueLength; x++ )
        {
            /* Ask how many messages are available... */
            uxReturn = uxQueueMessagesWaiting( xQueue );

            /* Check the number of messages being reported as being available
            is as expected, and force an assert if not. */
            if( uxReturn != x )
            {
                /* xQueue cannot be NULL so this is deliberately causing an
                assert to be triggered as there is an error. */
                configASSERT( xQueue == NULL );
            }

            /* Ask how many spaces remain in the queue... */
            uxReturn = uxQueueSpacesAvailable( xQueue );

            /* Check the number of spaces being reported as being available
            is as expected, and force an assert if not. */
            if( uxReturn != ( uxQueueLength - x ) )
            {
                /* xQueue cannot be NULL so this is deliberately causing an
                assert to be triggered as there is an error. */
                configASSERT( xQueue == NULL );
            }

            /* Fill one more space in the queue. */
            xQueueSendToBack( xQueue, NULL, 0 );
        }

        /* Perform the same check while the queue is full. */
        uxReturn = uxQueueMessagesWaiting( xQueue );
        if( uxReturn != uxQueueLength )
        {
            configASSERT( xQueue == NULL );
        }

        uxReturn = uxQueueSpacesAvailable( xQueue );

        if( uxReturn != 0 )
        {
            configASSERT( xQueue == NULL );
        }

        /* The queue is full, start again. */
        xQueueReset( xQueue );

#if( configUSE_PREEMPTION == 0 )
        taskYIELD();
#endif
    }
}
/*-----------------------------------------------------------*/

static void prvPermanentlyBlockingSemaphoreTask( void *pvParameters )
{
    SemaphoreHandle_t xSemaphore;

    /* Prevent compiler warning about unused parameter in the case that
    configASSERT() is not defined. */
    ( void ) pvParameters;

    /* This task should block on a semaphore, and never return. */
    xSemaphore = xSemaphoreCreateBinary();
    configASSERT( xSemaphore );

    xSemaphoreTake( xSemaphore, portMAX_DELAY );

    /* The above xSemaphoreTake() call should never return, force an assert if
    it does. */
    configASSERT( pvParameters != NULL );
    vTaskDelete( NULL );
}
/*-----------------------------------------------------------*/

static void prvPermanentlyBlockingNotificationTask( void *pvParameters )
{
    /* Prevent compiler warning about unused parameter in the case that
    configASSERT() is not defined. */
    ( void ) pvParameters;

    /* This task should block on a task notification, and never return. */
    ulTaskNotifyTake( pdTRUE, portMAX_DELAY );

    /* The above ulTaskNotifyTake() call should never return, force an assert
    if it does. */
    configASSERT( pvParameters != NULL );
    vTaskDelete( NULL );
}
