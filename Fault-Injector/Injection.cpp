#include "Injection.h"

Injection::Injection(long long pid, DataStructure ds, long long max_time_ms) : ds(ds) {
    this->pid = pid;
	this->max_time_ms = max_time_ms;
	this->random_time_ms = rand() % max_time_ms;
}

#if defined __linux__
    void Injection::inject(std::chrono::steady_clock::time_point begin_time) {
        struct iovec local[1];
        struct iovec remote[1];
        ssize_t nread;
        ssize_t nwrite;
        pid_t sim_pid = this->pid;

        char byte_buffer;

        local[0].iov_base = &byte_buffer;
        local[0].iov_len = 1;

        int target_byte_number = rand() % 168;
        int target_bit_number = rand() % 8;

        printf("\tTarget byte number: %d\n", target_byte_number);
        printf("\tTarget bit number: %d\n", target_bit_number);

        // Read the byte
        remote[0].iov_base = (void*)((char*)this->ds.get_address() + target_byte_number);
        remote[0].iov_len = 1;

        // Wait
        printf("\trandom_time_ms: %lld\n", this->random_time_ms );
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - begin_time).count();
        printf("\twaiting ms %lld\n", this->random_time_ms - ms);
        std::this_thread::sleep_for(std::chrono::milliseconds(this->random_time_ms - ms));
        printf("\tafter waiting..\n");

        // Injection
        nread = process_vm_readv(sim_pid, local, 1, remote, 1, 0);
        if (nread == -1) {
            std::cerr << "Can't read simulator memory" << std::endl;
            exit(1);
        }

        printf("\tchar read before injection: %d\n", byte_buffer);

        // Flip bit
        byte_buffer = byte_buffer ^ (1 << target_bit_number);

        printf("\tTheoretical injected byte: %d\n", byte_buffer);

        // Write the byte
        nwrite = process_vm_writev(sim_pid, local, 1, remote, 1, 0);
        if (nwrite == -1) {
            std::cerr << "Can't write simulator memory" << std::endl;
            exit(1);
        }

        // Debug
        nread = process_vm_readv(sim_pid, local, 1, remote, 1, 0);
        if (nread == -1) {
            std::cerr << "Can't read simulator memory" << std::endl;
            exit(1);
        }

        printf("\tchar read after injection: %d\n", byte_buffer);
    }

#elif defined _WIN32
    void Injection::inject(std::chrono::steady_clock::time_point begin_time) {
        HANDLE simProc;
        SIZE_T nread;
        SIZE_T nwrite;

        char byte_buffer = 0;

        int target_byte_number = rand() % 168;
        int target_bit_number = rand() % 8;

        // Open simulator Process, select the target byte and compute its address
        simProc = OpenProcess(PROCESS_ALL_ACCESS, false, this->pid);

        printf("\tTarget byte number: %d\n", target_byte_number);
        printf("\tTarget bit number: %d\n", target_bit_number);

        char* target_address = (char*)this->ds.get_address() + target_byte_number;

        // Wait
        printf("\trandom_time_ms: %lld\n", this->random_time_ms);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - begin_time).count();
        printf("\twaiting ms %lld\n", this->random_time_ms - ms);
        std::this_thread::sleep_for(std::chrono::milliseconds(this->random_time_ms - ms));
        printf("\tafter waiting..\n");

        // Injection
        // Read the byte
        ReadProcessMemory(simProc, (void*)target_address, &byte_buffer, 1, &nread);
        if (nread == 0) {
            std::cerr << "Can't read simulator memory" << std::endl;
            exit(1);
        }

        printf("\tchar read before injection: %d\n", byte_buffer);

        // Flip bit
        byte_buffer = byte_buffer ^ (1 << target_bit_number);

        printf("\tTheoretical injected byte: %d\n", byte_buffer);

        // Write the byte
        WriteProcessMemory(simProc, (void*)target_address, &byte_buffer, 1, &nwrite);
        if (nwrite == 0) {
            std::cerr << "Can't write simulator memory" << std::endl;
            exit(1);
        }

        // Debug
        ReadProcessMemory(simProc, (void*)target_address, &byte_buffer, 1, &nread);
        if (nread == 0) {
            std::cerr << "Can't read simulator memory" << std::endl;
            exit(1);
        }

        printf("\tchar read after injection: %d\n", byte_buffer);

        CloseHandle(simProc);
    }
#endif
