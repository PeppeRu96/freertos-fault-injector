#include "Injection.h"

Injection::Injection(long long pid, DataStructure ds, unsigned long max_time_ms) : ds(ds) {
    this->pid = pid;
	this->max_time_ms = max_time_ms;
	this->random_time_ms = rand() % max_time_ms;
    this->target_bit_number = rand() % 8;

#if defined __linux__
    this->linux_pid = pid;
#elif defined _WIN32
    this->sim_proc_handle = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
    this->handle_open = true;
#endif
}

Injection::~Injection() {
#if defined _WIN32
    if (handle_open)
        CloseHandle(this->sim_proc_handle);
#endif
}

void Injection::init() {

}

void Injection::close() {
#if defined _WIN32
    if (handle_open) {
        CloseHandle(this->sim_proc_handle);
        handle_open = false;
    }
#endif
}

void Injection::inject(std::chrono::steady_clock::time_point begin_time) {
    // Temporary
    target_byte_number = rand() % ds.get_fixed_size();
    char* target_address = (char*)ds.get_address() + target_byte_number;

    // Wait
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - begin_time).count();
    std::this_thread::sleep_for(std::chrono::milliseconds(random_time_ms - ms));

    // Injection
    // 1. Read phase
    read_memory((void*)target_address, &byte_buffer_before, 1);

    // 2. Flip phase
    byte_buffer_after = byte_buffer_before ^ (1 << target_bit_number);

    // 3. Write phase
    write_memory((void*)target_address, &byte_buffer_after, 1);
}

void Injection::print_stats() {
    using namespace std;

    cout << "Injection stats:" << endl;
    cout << "Performed after " << random_time_ms << " ms from the FreeRTOS simulator scheduler start" << endl;
    cout << "Target byte: " << target_byte_number << endl;
    cout << "Target bit: " << target_bit_number << endl;
    cout << "Byte value as unsigned integer before injection: " << (unsigned int)byte_buffer_before << endl;
    cout << "Byte value as unsigned integer after injection: " << (unsigned int)byte_buffer_after << endl;
}


// Low level read/write memory (platform-dependent)
#if defined __linux
void Injection::read_memory(void* address, char* buffer, size_t size) {
    struct iovec local[1];
    struct iovec remote[1];
    ssize_t nread;

    local[0].iov_base = buffer;
    local[0].iov_len = size;

    remote[0].iov_base = address;
    remote[0].iov_len = size;

    nread = process_vm_readv(this->linux_pid, local, 1, remote, 1, 0);
    if (nread == -1) {
        std::cerr << "Can't read simulator memory" << std::endl;
        exit(1);
    }
}
void Injection::write_memory(void* address, char* buffer, size_t size) {
    struct iovec local[1];
    struct iovec remote[1];
    ssize_t nwrite;

    local[0].iov_base = buffer;
    local[0].iov_len = size;

    remote[0].iov_base = address;
    remote[0].iov_len = size;

    nwrite = process_vm_writev(this->linux_pid, local, 1, remote, 1, 0);
    if (nwrite == -1) {
        std::cerr << "Can't write simulator memory" << std::endl;
        exit(1);
    }
}
#elif defined _WIN32
void Injection::read_memory(void* address, char* buffer, size_t size) {
    SIZE_T nread;

    ReadProcessMemory(this->sim_proc_handle, address, buffer, size, &nread);
    if (nread == 0) {
        std::cerr << "Can't read simulator memory" << std::endl;
        exit(1);
    }
}
void Injection::write_memory(void* address, char* buffer, size_t size) {
    SIZE_T nwrite;

    char* target_address = (char*)address;

    WriteProcessMemory(this->sim_proc_handle, address, buffer, size, &nwrite);
    if (nwrite == 0) {
        std::cerr << "Can't write simulator memory" << std::endl;
        exit(1);
    }
}
#endif
/*
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

        int target_byte_number = rand() % this->ds.get_fixed_size();
        int target_bit_number = rand() % 8;

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

        std::cout << "\tData Structure fixed size: " << this->ds.get_fixed_size() << std::endl;
        printf("\tTarget byte number: %d\n", target_byte_number);
        printf("\tTarget bit number: %d\n", target_bit_number);
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
        char buffer[500];

        int target_byte_number = rand() % this->ds.get_fixed_size();
        int target_bit_number = rand() % 8;

        // Open simulator Process, select the target byte and compute its address
        simProc = OpenProcess(PROCESS_ALL_ACCESS, false, this->pid);

        char* target_address = (char*)this->ds.get_address() + target_byte_number;
        char* struct_address = (char*)this->ds.get_address();

        // Wait
        printf("\trandom_time_ms: %lld\n", this->random_time_ms);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - begin_time).count();
        printf("\twaiting ms %lld\n", this->random_time_ms - ms);
        std::this_thread::sleep_for(std::chrono::milliseconds(this->random_time_ms - ms));
        printf("\tafter waiting..\n");

        // Debug
        ReadProcessMemory(simProc, (void*)struct_address, &buffer, this->ds.get_fixed_size(), &nread);
        if (nread == 0) {
            std::cerr << "Can't read simulator memory" << std::endl;
            exit(1);
        }
        std::cout << "\tQueue fields before injection:" << std::endl;
        test_print((void*)buffer);

        // Injection
        // Read the byte
        ReadProcessMemory(simProc, (void*)target_address, &byte_buffer, 1, &nread);
        if (nread == 0) {
            std::cerr << "Can't read simulator memory" << std::endl;
            exit(1);
        }
        

        std::cout << "\tData Structure fixed size: " << this->ds.get_fixed_size() << std::endl;
        printf("\tTarget byte number: %d\n", target_byte_number);
        printf("\tTarget bit number: %d\n", target_bit_number);
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

        // Debug
        ReadProcessMemory(simProc, (void*)struct_address, &buffer, this->ds.get_fixed_size(), &nread);
        if (nread == 0) {
            std::cerr << "Can't read simulator memory" << std::endl;
            exit(1);
        }
        std::cout << "\tQueue fields after injection:" << std::endl;
        test_print((void*)buffer);

        CloseHandle(simProc);
    }
#endif
*/
