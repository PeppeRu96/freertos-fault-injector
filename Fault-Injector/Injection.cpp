#include "Injection.h"

Injection::Injection(long long pid, DataStructure ds, long long max_time_ms) : ds(ds) {
    this->pid = pid;
	this->max_time_ms = max_time_ms;
	this->random_time_ms = rand() % max_time_ms;
}

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

    printf("Target byte number: %d\n", target_byte_number);
    printf("Target bit number: %d\n", target_bit_number);

    // Wait
    printf("random_time_ms: %lld\n", this->random_time_ms );
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - begin_time).count();
    printf("waiting ms %lld\n", this->random_time_ms - ms);
    std::this_thread::sleep_for(std::chrono::milliseconds(this->random_time_ms - ms));
    printf("after waiting..");

    // Injection
    // Read the byte
    remote[0].iov_base = (void *)((char *)this->ds.get_address() + target_byte_number);
    remote[0].iov_len = 1;

    nread = process_vm_readv(sim_pid, local, 1, remote, 1, 0);
    if (nread == -1) {
        std::cerr << "Can't read simulator memory" << std::endl;
        exit(1);
    }

    printf("char read before injection: %d\n", byte_buffer);

    // Flip bit
    byte_buffer = byte_buffer ^ (1 << target_bit_number);

    printf("Theoretical injected byte: %d\n", byte_buffer);

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

    printf("char read after injection: %d\n", byte_buffer);
}
