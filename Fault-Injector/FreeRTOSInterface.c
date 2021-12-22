#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "memory_logger.h"

size_t get_fixed_sizeof_struct(int type) {
	if (type == TYPE_TASK_HANDLE) {
		return getTCB_FixedSize();
	}
	else if (type == TYPE_QUEUE_HANDLE) {
		return getQueue_FixedSize();
	}
	else if (type == TYPE_TIMER_HANDLE) {

	}
	else if (type == TYPE_SEMAPHORE_HANDLE) {

	}
	else if (type == TYPE_COUNT_SEMAPHORE) {

	}
	else if (type == TYPE_EVENT_GROUP_HANDLE) {

	}
	else if (type == TYPE_MESSAGE_BUFFER_HANDLE) {

	}
	else if (type == TYPE_STREAM_BUFFER_HANDLE) {

	}
	else if (type == TYPE_QUEUE_SET_HANDLE) {

	}
	else if (type == TYPE_STATIC_STACK) {

	}
	return 0;
}

void test_print(void* addr) {
	printQueueFields((QueueHandle_t)addr);
}



/* Just to implement some required FreeRTOS functions */
void vAssertCalled(unsigned long ulLine, const char* const pcFileName)
{
}
void vAssertCalledM(unsigned long ulLine, const char* const pcFileName, const char* const message)
{
}
void vApplicationMallocFailedHook(void)
{
}
void vApplicationIdleHook(void)
{
}
void vApplicationTickHook(void)
{
}
void vApplicationGetIdleTaskMemory(StaticTask_t** ppxIdleTaskTCBBuffer, StackType_t** ppxIdleTaskStackBuffer, uint32_t* pulIdleTaskStackSize)
{
}
void vApplicationGetTimerTaskMemory(StaticTask_t** ppxTimerTaskTCBBuffer, StackType_t** ppxTimerTaskStackBuffer, uint32_t* pulTimerTaskStackSize)
{
}