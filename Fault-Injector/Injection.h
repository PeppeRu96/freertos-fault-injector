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
#endif

#include "DataStructure.h"

class Injection {
private:
	DataStructure ds;
    long long pid;

	char original_byte;
	char injected_byte;

	long long max_time_ms;
	long long random_time_ms;

public:
	Injection(long long pid, DataStructure ds, long long max_time_ms);

	void inject(std::chrono::steady_clock::time_point begin_time);
};

#endif //FREERTOS_FAULTINJECTOR_INJECTION_H
