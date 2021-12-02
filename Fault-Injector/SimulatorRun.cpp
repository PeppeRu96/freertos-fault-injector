//
// Created by ruggeri on 02/12/21.
//

#include "SimulatorRun.h"

void SimulatorRun::start(std::string sim_path) {
    bp::child new_child(sim_path, bp::std_out > this->pipe_stream);
    this->begin_time = std::chrono::steady_clock::now();
    this->c = std::move(new_child);
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

void SimulatorRun::show() {
    std::string line;
    while (this->pipe_stream && std::getline(this->pipe_stream, line) && !line.empty())
        std::cerr << line << std::endl;
}
