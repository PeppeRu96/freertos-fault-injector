//
// Created by ruggeri on 02/12/21.
//

#ifndef FREERTOS_FAULTINJECTOR_SIMULATORRUN_H
#define FREERTOS_FAULTINJECTOR_SIMULATORRUN_H

#include <iostream>
#include <algorithm>
#include <chrono>
#include <string>
#include <boost/process.hpp>

namespace bp = boost::process;

class SimulatorRun {
private:
    bp::child c;
    bp::ipstream pipe_stream;

    std::chrono::steady_clock::time_point begin_time;
    std::chrono::steady_clock::time_point end_time;

public:
    void start(std::string sim_path);
    auto duration();
    std::error_code wait();
    void terminate();
    void show();
};


#endif //FREERTOS_FAULTINJECTOR_SIMULATORRUN_H
