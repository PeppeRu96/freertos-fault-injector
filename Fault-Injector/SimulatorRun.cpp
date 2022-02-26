//
// Created by ruggeri on 02/12/21.
//
//
#include "SimulatorRun.h"

#include "loguru.hpp"
#include <sstream>

SimulatorRun::SimulatorRun() {
    this->loaded_duration = nullptr;
    this->error_matched_str = "";
    this->delayed_str = "";
    this->delay_amount = 0;
}

SimulatorRun::~SimulatorRun() {
    std::string pid = std::to_string(this->c.id());
    std::string sem1_name = "binary_sem_log_struct_" + pid + "_1";
    std::string sem2_name = "binary_sem_log_struct_" + pid + "_2";

    boost::interprocess::shared_memory_object::remove(sem1_name.c_str());
    boost::interprocess::shared_memory_object::remove(sem2_name.c_str());
}

void SimulatorRun::init(std::string sim_path) {
    bp::child new_child(sim_path);
    this->c = std::move(new_child);

    std::string pid = std::to_string(this->c.id());
    std::string sem1_name = "binary_sem_log_struct_" + pid + "_1";

    bi::named_semaphore s1(bi::open_or_create, sem1_name.c_str(), 0);

    // Wait that the data structures are ready to be read
    s1.wait();

    // Read data structures
    this->read_data_structures();
}

void SimulatorRun::start() {
    std::string pid = std::to_string(this->c.id());
    std::string sem2_name = "binary_sem_log_struct_" + pid + "_2";

    bi::named_semaphore s2(bi::open_or_create, sem2_name.c_str(), 0);
    
    // Signal to the simulator that it can start
    s2.post();

    this->begin_time = std::chrono::steady_clock::now();
}

void SimulatorRun::read_data_structures() {
    std::string s1 = MEM_LOG_FILE_PREFIX;
    std::string s2 = std::to_string(this->c.id());
    std::string s3 = ".txt";
    std::string path = "tmp/" + s1 + s2 + s3;
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
    //std::cout << buffer;
    while (fscanf(structures_log_fp, "%d %s %d %p\n", &struct_id, struct_name, &struct_type, &struct_address) == 4) {
        // printf("Id: %d, Name: %s, Type: %d, Address: %p\n", struct_id, struct_name, struct_type, struct_address);
        DataStructure ds(struct_id, struct_name, struct_type, struct_address);
        this->data_structures.push_back(ds);
    }

    // Debug
    /*
    for (auto const& ds : this->data_structures) {
        std::cout << ds << std::endl;
    }
    */

    fclose(structures_log_fp);
}

std::error_code SimulatorRun::wait() {
    std::error_code error;
    this->c.wait(error);
    this->end_time = std::chrono::steady_clock::now();

    return error;
}

bool SimulatorRun::wait_for(const std::chrono::steady_clock::duration& rel_time, std::error_code& ec) {
    bool time_has_not_expired = this->c.wait_for(rel_time, ec);
    this->end_time = std::chrono::steady_clock::now();

    return time_has_not_expired;
}

std::chrono::steady_clock::duration SimulatorRun::duration() {
    if (this->loaded_duration != nullptr)
        return *(this->loaded_duration);

    return (this->end_time - this->begin_time);
}

void SimulatorRun::load_duration(unsigned long ms) {
    auto dur = std::chrono::milliseconds(ms);
    this->loaded_duration = new std::chrono::steady_clock::duration(dur);
}

void SimulatorRun::terminate() {
    this->c.terminate();
}

void SimulatorRun::save_output(int* pid) {
    std::ifstream output_file;
    std::string pid_str = pid != nullptr ? std::to_string(*pid) : std::to_string(this->c.id());
    std::string output_f_pref = OUTPUT_FILE_PREFIX;
    std::string path = "output/" + output_f_pref + pid_str + ".txt";
    std::string line;

    output_file.open(path);
    if (output_file.is_open()) {
        while (std::getline(output_file, line))
            this->output.push_back(line);
    }
    else {
        std::cout << "Unable to open " << path << std::endl;
    }

    output_file.close();

    // Debug
    /*
    for (auto const& s : this->output)
        std::cout << s << std::endl;
    */
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

void SimulatorRun::print_stats(bool use_logger) {
    using namespace std;

    stringstream ss;

    ss << "Simulator run (PID " << this->get_pid() << ") stats:\n";
    ss << "Native exit code: " << this->get_native_exit_code() << std::endl;
    ss << "Execution took " << std::chrono::duration_cast<std::chrono::seconds>(this->duration()).count() << " seconds." << std::endl;

    if (use_logger) {
        RAW_LOG_F(INFO, ss.str().c_str());
    }
    else {
        cout << ss.str() << endl;
    }
}

SimulatorError SimulatorRun::compare_with_golden(const SimulatorRun& golden, std::string error_pattern) {
    // Output out-of-order -> Delay
    // Output different -> SDC
    // Output equal -> Masked

    if (this->output.size() != golden.output.size())
        return SDC;

    bool masked = true;
    bool sdc = false;
    for (int i = 0; i < this->output.size(); i++) {
        if (this->output[i] == golden.output[i])
            continue;
        masked = false;
        bool out_of_order = false;
        if (sdc == false) {
            for (int j = 0; j < golden.output.size(); j++) {
                if (this->output[i] == golden.output[j]) {
                    if (j < i)
                        if (this->delay_amount == 0 || this->delay_amount < i - j) {
                            this->delayed_str = this->output[i];
                            this->delay_amount = i - j;
                        }
                    out_of_order = true;
                    break;
                }
            }
        }
        if (out_of_order == false) {
            std::string tmp1 = this->output[i];
            std::string tmp2 = error_pattern;

            std::for_each(tmp1.begin(), tmp1.end(), [](char& c) {
                c = ::toupper(c);
                });
            std::for_each(tmp2.begin(), tmp2.end(), [](char& c) {
                c = ::toupper(c);
                });

            if (error_pattern == "") {
                this->error_matched_str = this->output[i];
                return SDC;
            }
            else if (tmp1.find(tmp2, 0) != -1) {
                this->error_matched_str = this->output[i];
                return SDC;
            }
            sdc = true;
        }
    }
    if (masked)
        return MASKED;
    if (sdc)
        return SDC;

    return DELAY;
}

std::vector<DataStructure> SimulatorRun::get_data_structures() const {
    return this->data_structures;
}

DataStructure SimulatorRun::get_ds_by_id(int id) const {
    for (auto ds : this->data_structures) {
        if (ds.get_id() == id)
            return ds;
    }
    std::cerr << "Error: Data structure with id: " << id << " not found." << std::endl;
    exit(2);
}

std::chrono::steady_clock::time_point SimulatorRun::get_begin_time() const {
    return this->begin_time;
}

long long SimulatorRun::get_pid() const {
    return this->c.id();
}

int SimulatorRun::get_native_exit_code() const {
    return this->c.native_exit_code();
}

bool SimulatorRun::is_running() {
    return this->c.running();
}

std::string SimulatorRun::get_error_matched_str() const {
    return this->error_matched_str;
}

std::string SimulatorRun::get_delayed_str() const {
    return this->delayed_str;
}

int SimulatorRun::get_delay_amount() const {
    return delay_amount;
}
