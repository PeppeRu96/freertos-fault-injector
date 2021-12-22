#ifndef FREERTOS_INTERFACE_H
#define FREERTOS_INTERFACE_H

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
