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
		return sizeof(StaticTimer_t);
	}
	else if (type == TYPE_SEMAPHORE_HANDLE) {
		return sizeof(StaticSemaphore_t);
	}
	else if (type == TYPE_COUNT_SEMAPHORE) {
		return sizeof(StaticQueue_t);
	}
	else if (type == TYPE_EVENT_GROUP_HANDLE) {
		return sizeof(StaticEventGroup_t);
	}
	else if (type == TYPE_MESSAGE_BUFFER_HANDLE) {
		return sizeof(StaticMessageBuffer_t);
	}
	else if (type == TYPE_STREAM_BUFFER_HANDLE) {
		return sizeof(StaticStreamBuffer_t);
	}
	else if (type == TYPE_QUEUE_SET_HANDLE) {
		return sizeof(StaticQueue_t);
	}
	else if (type == TYPE_STATIC_STACK) {
		return configMINIMAL_STACK_SIZE * 2;
	}
	return 0;
}

size_t get_exploded_sizeof_struct(int type, void* ds) {
	if (type == TYPE_TASK_HANDLE) {
		return getTCB_FixedSize();
	}
	else if (type == TYPE_QUEUE_HANDLE) {
		return getQueue_CurrentExplodedSize((QueueHandle_t)ds);
	}
	else if (type == TYPE_TIMER_HANDLE) {
		return sizeof(StaticTimer_t);
	}
	else if (type == TYPE_SEMAPHORE_HANDLE) {
		return sizeof(StaticSemaphore_t);
	}
	else if (type == TYPE_COUNT_SEMAPHORE) {
		return sizeof(StaticQueue_t);
	}
	else if (type == TYPE_EVENT_GROUP_HANDLE) {
		return sizeof(StaticEventGroup_t);
	}
	else if (type == TYPE_MESSAGE_BUFFER_HANDLE) {
		return sizeof(StaticMessageBuffer_t);
	}
	else if (type == TYPE_STREAM_BUFFER_HANDLE) {
		return sizeof(StaticStreamBuffer_t);
	}
	else if (type == TYPE_QUEUE_SET_HANDLE) {
		return sizeof(StaticQueue_t);
	}
	else if (type == TYPE_STATIC_STACK) {
		return configMINIMAL_STACK_SIZE * 2;
	}
	return 0;
}

void get_next_expansion_struct(int type, void* ds, size_t byte_number, void** byte_to_inject, void** addr_to_read, size_t* size_to_read) {
	if (type == TYPE_TASK_HANDLE) {
	}
	else if (type == TYPE_QUEUE_HANDLE) {
		getQueue_NextExpansion((QueueHandle_t)ds, byte_number, byte_to_inject, addr_to_read, size_to_read);
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