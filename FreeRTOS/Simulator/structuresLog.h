#include <stdlib.h>
#include <stdio.h>

#ifndef INCLUDE_STRUCTURES_LOG_H
#define INCLUDE_STRUCTURES_LOG_H

/* define the types of structures */
#define TYPE_TASK_HANDLE 0
#define TYPE_QUEUE_HANDLE 1
#define TYPE_TIMER_HANDLE 2
#define TYPE_SEMAPHORE_HANDLE 3
#define TYPE_COUNT_SEMAPHORE 4
#define TYPE_EVENT_GROUP_HANDLE 5
#define TYPE_MESSAGE_BUFFER_HANDLE 6
#define TYPE_STREAM_BUFFER_HANDLE 7
#define TYPE_QUEUE_SET_HANDLE 8
#define TYPE_STATIC_STACK 9


void log_struct(char* name, int type, void* address);
void log_start();
void log_end();

#endif
