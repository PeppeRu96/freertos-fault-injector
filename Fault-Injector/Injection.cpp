#include "Injection.h"

#if defined __unix__
#include <unistd.h>
#include <sys/uio.h>
#include <errno.h>
#elif defined _WIN32
#include <Windows.h>
#endif

#include "FreeRTOSInterface.h"

Injection::Injection(SimulatorRun* sr, DataStructure ds, unsigned long max_time_ms) : ds(ds) {
    this->sr = sr;
    this->pid = sr->get_pid();
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
    // Wait
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - begin_time).count();
    std::this_thread::sleep_for(std::chrono::milliseconds(random_time_ms - ms));

    // Injection
    // Check if the Simulator is still running (it may have crashed in the meanwhile..)
    // However, in a normal situation, theoretically a non-injected simulator should not crash
    if (!sr->is_running())
        return;

    // 1. Read phase
    // Read the entire data structure
    char* struct_before = ds.get_struct_before();
    read_memory(ds.get_address(), struct_before, ds.get_fixed_size());
    std::cout << "Before injection queue:" << std::endl;
    std::cout << "----------------------" << std::endl;
    test_print(struct_before);
    // Get the exploded data structure size (including items stored in lists etc.)
    exploded_size = ds.get_exploded_size();

    // Next, generate a random number in the virtual exploded size space
    target_byte_number = rand() % exploded_size;

    // Then, analyze FreeRTOS data structure to check where we are pointing with our random byte number:
    //  1: If we are pointing to a field which does not need any expansion, we select this byte for the injeciton;
    //  2: If we are pointing to a field in the expanded space, we need to follow some pointers to retrieve the real byte address to inject
    if (target_byte_number < ds.get_fixed_size()) {
        // 1
        injected_byte_addr = (void *)( (char*)ds.get_address() + target_byte_number );
        byte_buffer_before = struct_before[target_byte_number];
    }
    else {
        // 2
        void* addr_to_read;
        size_t size_to_read;
        ds.get_next_expansion(target_byte_number - ds.get_fixed_size(), &injected_byte_addr, &addr_to_read, &size_to_read);
        if (addr_to_read == NULL) {
            read_memory(injected_byte_addr, &byte_buffer_before, 1);
        }
        else {
            // TODO (deeper linking)
        }
    }

    // 2. Flip phase
    byte_buffer_after = byte_buffer_before ^ (1 << target_bit_number);

    // 3. Write phase
    write_memory(injected_byte_addr, &byte_buffer_after, 1);

    char struct_after[500];
    read_memory(ds.get_address(), struct_after, ds.get_fixed_size());

    std::cout << "\n----------------------------" << std::endl;
    std::cout << "Queue after the injection: " << std::endl;
    test_print(struct_after);
    std::cout << "\n" << std::endl;
}

void Injection::print_stats() {
    using namespace std;

    cout << "Injection stats:" << endl;
    cout << "Performed after " << random_time_ms << " ms from the FreeRTOS simulator scheduler start" << endl;
    cout << "Target data structure size (bytes): " << ds.get_fixed_size() << endl;
    cout << "Target data structure expanded size (bytes): " << ds.get_exploded_size() << endl;
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
