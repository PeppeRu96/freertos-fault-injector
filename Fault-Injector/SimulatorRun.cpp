//
// Created by ruggeri on 02/12/21.
//
//
#include "SimulatorRun.h"

void SimulatorRun::start(std::string sim_path) {
    bp::child new_child(sim_path);
    this->c = std::move(new_child);

    std::string pid = std::to_string(this->c.id());
    std::string sem1_name = "binary_sem_log_struct_" + pid + "_1";
    std::string sem2_name = "binary_sem_log_struct_" + pid + "_2";

    bi::named_semaphore s1(bi::open_or_create, sem1_name.c_str(), 0);
    bi::named_semaphore s2(bi::open_or_create, sem2_name.c_str(), 0);
    
    // Wait that the data structures are ready to be read
    s1.wait();

    // Read data structures
    this->read_data_structures();
    
    // Signal to the simulator that it can start
    s2.post();

    this->begin_time = std::chrono::steady_clock::now();
}

void SimulatorRun::read_data_structures() {
    std::string s1 = MEM_LOG_FILE_PREFIX;
    std::string s2 = std::to_string(this->c.id());
    std::string s3 = ".txt";
    std::string path = s1 + s2 + s3;
    FILE *structures_log_fp = fopen(path.c_str(), "r");

    if (structures_log_fp == NULL) {
        std::cerr << "Error while opening the file " << path << " for reading the data structures." << std::endl;
        exit(1);
    }

    int struct_id;
    char struct_name[100];
    int struct_type;
    void *struct_address;
    char buffer[100];
    int n_read;

    fgets(buffer, 100, structures_log_fp);
    std::cout << buffer;
    while (fscanf(structures_log_fp, "%d %s %d %p\n", &struct_id, struct_name, &struct_type, &struct_address) == 4) {
        printf("Id: %d, Name: %s, Type: %d, Address: %p\n", struct_id, struct_name, struct_type, struct_address);
    }
}

std::error_code SimulatorRun::wait() {
    std::error_code error;
    this->c.wait(error);

    return error;
}

auto SimulatorRun::duration() {
    return (this->end_time - this->begin_time).count();
}

void SimulatorRun::terminate() {
    this->c.terminate();
}

void SimulatorRun::show_output() {
    std::ifstream output_file;
    std::string path = OUTPUT_FILE_PREFIX + std::to_string(this->c.id()) + ".txt";
    std::string line;

    output_file.open(path);
    if (output_file.is_open()) {
        while (std::getline(output_file, line))
            std::cout << line << std::endl;
    }
    else {
        std::cout << "Unable to open " << path << std::endl;
    }
       
    output_file.close();
}
