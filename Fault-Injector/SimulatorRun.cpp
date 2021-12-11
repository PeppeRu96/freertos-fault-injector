//
// Created by ruggeri on 02/12/21.
//
//
#include "SimulatorRun.h"

void SimulatorRun::start(std::string sim_path) {
    //bp::child new_child(sim_path, (bp::std_out & bp::std_err) > this->pipe_stream);
    bp::child new_child(sim_path);
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
