#ifndef MEMORY_LOGGER_H
#define MEMORY_LOGGER_H

/* define the encoding for the types of the structures */
enum DataStructureType {
    TYPE_TASK_HANDLE,
    TYPE_QUEUE_HANDLE,
    TYPE_TIMER_HANDLE,
    TYPE_SEMAPHORE_HANDLE,
    TYPE_COUNT_SEMAPHORE,
    TYPE_EVENT_GROUP_HANDLE,
    TYPE_MESSAGE_BUFFER_HANDLE,
    TYPE_STREAM_BUFFER_HANDLE,
    TYPE_QUEUE_SET_HANDLE,
    TYPE_STATIC_STACK
};


#ifdef __cplusplus
extern "C" {
#endif

    void log_struct(char *name, int type, void *address);
    void log_data_structs_start();
    void log_data_structs_end();

#ifdef __cplusplus
}
#endif

#endif /* MEMORY_LOGGER_H */