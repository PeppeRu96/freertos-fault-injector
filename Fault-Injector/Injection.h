//
// Created by ruggeri on 15/12/21.
//

#ifndef FREERTOS_FAULTINJECTOR_INJECTION_H
#define FREERTOS_FAULTINJECTOR_INJECTION_H

#include <chrono>
#include <thread>

#if defined __unix__
    #include <unistd.h>
    #include <sys/uio.h>
    #include <errno.h>
#elif defined _WIN32
	#include <Windows.h>
#endif

#include "DataStructure.h"

class Injection {
private:
	DataStructure ds;
    long long pid;

	unsigned long max_time_ms;
	unsigned long random_time_ms;

	// Injection structures
	char byte_buffer_before;
	char byte_buffer_after;
	char struct_buffer_before[500];
	char struct_buffer_after[500];
	unsigned int target_byte_number;
	unsigned short target_bit_number;

#if defined __linux__
	pid_t linux_pid;
#elif defined _WIN32
	bool handle_open;
	HANDLE sim_proc_handle;
#endif

	void read_memory(void* address, char* buffer, size_t size);
	void write_memory(void* address, char* buffer, size_t size);

public:
	Injection(long long pid, DataStructure ds, unsigned long max_time_ms);
	~Injection();

	void init();
	void inject(std::chrono::steady_clock::time_point begin_time);
	void close();

	void print_stats();
};

#endif //FREERTOS_FAULTINJECTOR_INJECTION_H
