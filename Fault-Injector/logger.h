#ifndef FREERTOS_FAULTINJECTOR_LOGGER_H
#define FREERTOS_FAULTINJECTOR_LOGGER_H

#include "loguru.hpp"

#include "SimulatorRun.h"
#include "Injection.h"

#include <string.h>

void log_init(loguru::FileMode f_mode, std::string* fname);

void log_injection_trial(SimulatorRun& golden, SimulatorRun& sr, Injection& inj, std::error_code ec, SimulatorError se, std::string error_pattern);

void log_join(std::string fname);

void create_data_dirs();

void remove_tmp();

#endif
