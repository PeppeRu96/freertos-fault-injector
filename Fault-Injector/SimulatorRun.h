//
// Created by ruggeri on 02/12/21.
//

#ifndef FREERTOS_FAULTINJECTOR_SIMULATORRUN_H
#define FREERTOS_FAULTINJECTOR_SIMULATORRUN_H

#include <iostream>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <string>
#include <boost/process.hpp>
#include <vector>

#include "simulator_config.h"

namespace bp = boost::process;

class SimulatorRun {
private:
    bp::child c;

    std::chrono::steady_clock::time_point begin_time;
    std::chrono::steady_clock::time_point end_time;

public:
    // Delete copy constructor and copy assignment
    // Allow only move constructor and assignment
    void start(std::string sim_path);
    auto duration();
    std::error_code wait();
    void terminate();
    void show_output();
};


#endif //FREERTOS_FAULTINJECTOR_SIMULATORRUN_H
