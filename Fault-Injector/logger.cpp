#include "logger.h"
#include <ctime>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <sstream>

void log_init() {
    loguru::g_stderr_verbosity = 1;
    loguru::g_preamble_thread = false; // The logging thread
    loguru::g_preamble_file = false; // The file from which the log originates from
    loguru::g_preamble_verbose = false; // The verbosity field

    std::time_t now = std::time(nullptr);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now), "%F_%T");
    auto s = ss.str();
    std::replace(s.begin(), s.end(), ':', '-');
    auto fname = "logs/log_" + s + ".log";
    loguru::add_file(fname.c_str(), loguru::Truncate, loguru::Verbosity_INFO);
}