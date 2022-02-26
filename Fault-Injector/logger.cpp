#include "logger.h"
#include <ctime>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdio.h>
#include <filesystem>

namespace fs = std::filesystem;

void log_init(loguru::FileMode f_mode, std::string* fname) {
    loguru::g_stderr_verbosity = 1;
    loguru::g_preamble_thread = false; // The logging thread
    loguru::g_preamble_file = false; // The file from which the log originates from
    loguru::g_preamble_verbose = false; // The verbosity field

    std::time_t now = std::time(nullptr);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now), "%F_%T");
    auto s = ss.str();
    std::replace(s.begin(), s.end(), ':', '-');
    auto f_path = "logs/log_" + s + ".log";
    if (fname != nullptr) {
        f_path = "logs/log_" + *fname + ".log";
    }
    loguru::add_file(f_path.c_str(), f_mode, loguru::Verbosity_INFO);
}

void log_injection_trial(SimulatorRun& golden, SimulatorRun& sr, Injection& inj, std::error_code ec, SimulatorError se, std::string error_pattern) {
    std::stringstream ss;
    
    sr.print_stats(true);
    RAW_LOG_F(INFO, "");
    inj.print_stats(true);
    RAW_LOG_F(INFO, "");

    switch (se) {
    case MASKED:
        RAW_LOG_F(INFO, "Simulator error:\t Masked");
        break;
    case SDC:
        RAW_LOG_F(INFO, "Simulator error:\t Silence Data Corruption");
        if (error_pattern != "") {
            if (sr.get_error_matched_str() != "") {
                RAW_LOG_F(INFO, "Search for specific errors containing the string: %s ...\nFound matching string: %s", error_pattern.c_str(), sr.get_error_matched_str().c_str());
            }
            else {
                RAW_LOG_F(INFO, "Search for specific errors containing the string: %s ...\nNot found\nUnexpected output: %s", error_pattern.c_str(), sr.get_error_matched_str().c_str());
            }
        }
        else {
            RAW_LOG_F(INFO, "Unexpected output: %s", sr.get_error_matched_str().c_str());
        }
        break;
    case DELAY:
        RAW_LOG_F(INFO, "Simulator error:\t Delay");
        RAW_LOG_F(INFO, "The injected FreeRTOS simulator has produced the following output with a delay of %d operations:", sr.get_delay_amount());
        RAW_LOG_F(INFO, "%s", sr.get_delayed_str().c_str());
        break;
    case HANG:
        RAW_LOG_F(INFO, "Simulator error:\t Hang");

        ss << std::chrono::duration_cast<std::chrono::seconds>(golden.duration() * DEADLOCK_TIME_FACTOR).count();
        
        RAW_LOG_F(INFO, "Simulator forcely killed after %s seconds (probable deadlock/spinlock)", ss.str().c_str());
        break;
    case CRASH:
        RAW_LOG_F(INFO, "Simulator error:\t Crash");

        ss << ec;
        RAW_LOG_F(INFO, "Error code: %s", ss.str().c_str());
        RAW_LOG_F(INFO, "Native exit code: %d", sr.get_native_exit_code());

        break;
    }
    RAW_LOG_F(INFO, "");
}

void log_join(std::string fname) {
    std::string f_path = "logs/log_" + fname + ".log";

    std::fstream other_log;
    other_log.open(f_path, std::ios::in);
    if (other_log.is_open()) {

        std::string line;

        while (getline(other_log, line)) {
            RAW_LOG_F(INFO, line.c_str());
        }

        other_log.close();

        // Remove file
        if (remove(f_path.c_str()) != 0) {
            std::cerr << "Error trying to delete file " << f_path << "\n";
        }
    }
    else {
        std::cerr << "Error opening the file " << f_path << " while trying to join two logs.\n";
    }

}

void create_data_dirs() {
    fs::create_directory("output");
    fs::create_directory("tmp");
}

void remove_tmp() {
    fs::remove_all("tmp");
}
