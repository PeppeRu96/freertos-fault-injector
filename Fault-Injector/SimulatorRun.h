//
// Created by ruggeri on 02/12/21.
//

#ifndef FREERTOS_FAULTINJECTOR_SIMULATORRUN_H
#define FREERTOS_FAULTINJECTOR_SIMULATORRUN_H

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <chrono>
#include <string>
#include <vector>
#include <boost/process.hpp>
#include <boost/process/extend.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/sync/named_semaphore.hpp>

#include "DataStructure.h"
#include "simulator_config.h"

#define DEADLOCK_TIME_FACTOR    2

namespace bp = boost::process;
namespace bi = boost::interprocess;

enum SimulatorError {
    MASKED,
    SDC,
    DELAY,
    HANG,
    CRASH
};

class SimulatorRun {
private:
    bp::child c;

    std::vector<DataStructure> data_structures;

    std::vector<std::string> output;

    std::chrono::steady_clock::time_point begin_time;
    std::chrono::steady_clock::time_point end_time;
    std::chrono::steady_clock::duration* loaded_duration;

    void read_data_structures();

public:
    SimulatorRun();
    ~SimulatorRun();

    // Delete copy constructor and copy assignment
    // Allow only move constructor and assignment
    void init(std::string sim_path);
    void start();
    std::chrono::steady_clock::duration duration();
    void load_duration(unsigned long ms);
    std::error_code wait();
    bool wait_for(const std::chrono::steady_clock::duration& rel_time, std::error_code& ec);
    void terminate();
    void save_output(int* pid);
    void show_output();
    void print_stats(bool use_logger);

    SimulatorError compare_with_golden(const SimulatorRun& golden);

    std::vector<DataStructure> get_data_structures() const;
    DataStructure get_ds_by_id(int id) const;
    std::chrono::steady_clock::time_point get_begin_time() const;
    long long get_pid() const;
    int get_native_exit_code() const;
    bool is_running();
};


#endif //FREERTOS_FAULTINJECTOR_SIMULATORRUN_H
