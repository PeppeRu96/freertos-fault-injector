#ifndef FREERTOS_INTERFACE_H
#define FREERTOS_INTERFACE_H

/*
* This provides an interface to the FreeRTOS internal data structures.
* Since the injection system copy the data structures from the FreeRTOS simulator
* instances to the local memory in order to perform the injection, we need a way
* to inject complex data structures (like Queues, Lists, and so on).
* Therefore, it is required to have control over the data structures to be injected.
*/

#ifdef __cplusplus
extern "C" {
#endif

	size_t get_fixed_sizeof_struct(int type);
	size_t get_exploded_sizeof_struct(int type, void* ds);
	void get_next_expansion_struct(int type, void* ds, size_t byte_number, void** byte_to_inject, void** addr_to_read, size_t* size_to_read);
	void test_print(void *addr);

#ifdef __cplusplus
}
#endif

#endif /* FREERTOS_INTERFACE_H */
