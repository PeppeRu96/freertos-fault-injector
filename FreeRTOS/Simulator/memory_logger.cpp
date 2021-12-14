#include "memory_logger.h"
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include <boost/interprocess/detail/os_thread_functions.hpp>
#include "simulator_config.h"

FILE *structures_log_fp;
int nextID = 0;

void log_struct(char *name, int type, void *address) {
    fprintf(structures_log_fp, "%d %s %d %p\n", nextID++, name, type, address);
}

void log_data_structs_start() {
    std::string s1 = MEM_LOG_FILE_PREFIX;
    std::string s2 = std::to_string(boost::interprocess::ipcdetail::get_current_process_id());
    std::string s3 = ".txt";
    std::string path = s1 + s2 + s3;
    structures_log_fp = fopen(path.c_str(), "w");
    if (structures_log_fp == NULL) {
        std::cerr << "Error opening the file " << path << "for writing the data structures" << std::endl;
        exit(1);
    }
    fprintf(structures_log_fp, "ID Name Type Address\n");
}

void log_data_structs_end() {
    fclose(structures_log_fp);
}