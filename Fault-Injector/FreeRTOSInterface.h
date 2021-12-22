#ifndef FREERTOS_INTERFACE_H
#define FREERTOS_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

	size_t get_fixed_sizeof_struct(int type);
	void test_print(void *addr);

#ifdef __cplusplus
}
#endif

#endif /* FREERTOS_INTERFACE_H */
