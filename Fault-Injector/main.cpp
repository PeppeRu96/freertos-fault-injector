#include <iostream>
#include <iomanip>
#include <string>
#include <time.h>
#include <algorithm>
#include <cstdlib>

#include "SimulatorRun.h"
#include "Injection.h"
#include "simulator_config.h"
#include "memory_logger.h"

#include "loguru.hpp"
#include "logger.h"

#define SIMULATOR_EXE_NAME      "FreeRTOS_Simulator"

std::string sim_exe_name = SIMULATOR_EXE_NAME;
std::string sim_path = sim_exe_name;

typedef struct {
    int struct_id;
    int inject_n;
    long long max_time_ms;
    bool parallelize;
    std::string error_pattern;
} InjectConf;

void menu(InjectConf &conf);

SimulatorRun golden_run;
std::error_code golden_run_ec;

void injection(InjectConf& conf);

void sequential_injections(InjectConf &conf);

void parallel_injections(InjectConf& conf, char* exe_name);

int main(int argc, char ** argv)
{
    InjectConf conf;

    create_data_dirs();

    // Check if this process has been created from another process (parallelization)
    int golden_run_pid;
    std::string curr_pid = std::to_string(boost::interprocess::ipcdetail::get_current_process_id());

    if (argc > 1) {
        // Parallel instance

        unsigned long golden_run_dur_ms;

        golden_run_pid = atoi(argv[1]);
        std::stringstream ss(argv[2]);
        ss >> golden_run_dur_ms;
        golden_run.load_duration(golden_run_dur_ms);

        int rand_seed = atoi(argv[3]);
        srand((unsigned)rand_seed);

        conf.struct_id = atoi(argv[4]);
        conf.inject_n = atoi(argv[5]);
        conf.max_time_ms = atoi(argv[6]);
        if (argc > 7)
            conf.error_pattern = atoi(argv[7]);
        else
            conf.error_pattern = "";

        // Open temp log
        log_init(loguru::Truncate, &curr_pid);

        // Load the golden run output
        golden_run.save_output(&golden_run_pid);

        // Start a simulation and inject
        injection(conf);
    }
    else {
        // Master instance
        srand((unsigned)time(NULL));

        std::cout << "######### FreeRTOS FaultInjector v" << PROJECT_VER << " #########" << std::endl;
        std::cout << std::endl;

        log_init(loguru::Truncate, nullptr);

        // Start a simulator and save the golden execution
        LOG_F(INFO, "Executing the simulator and saving the golden execution...");

        golden_run.init(sim_path);
        golden_run.start();
        golden_run_ec = golden_run.wait();
        golden_run.save_output(nullptr);
        RAW_LOG_F(INFO, "Golden run stats:");
        golden_run.print_stats(true);

        // Display user menu
        menu(conf);

        // Perform injections
        LOG_F(INFO, "-- Injections start --");
        if (!conf.parallelize)
            sequential_injections(conf);
        else
            parallel_injections(conf, argv[0]);

    }

    remove_tmp();

    return 0;
}

void menu(InjectConf &conf) {
    using namespace std;
    int op;
    string parallelize_str;
    string pattern_error_str;

    while (true) {
        cout << endl;
        cout << "Menu" << endl;
        cout << "0 - Show the golden execution output" << endl;
        cout << "1 - Perform some injections" << endl;
        cout << "Please, select an operation [0, 1]: ";
        cin >> op;
        cout << endl;

        if (op == 0) {
            cout << "Golden execution output:" << endl;
            golden_run.show_output();
            cout << endl;
            continue;
        }
        else if (op != 1) {
            cout << "Invalid option. Try again." << endl;
            continue;
        }

        cout << "-- Injection configuration --" << endl;
        cout << "Injectable data structures:" << endl;
        cout << left << setw(20) << "\tId" << left << setw(40) << "Name" << left << setw(20) << "Type" << endl;
        cout << "-------------------------------------------------------------------------------------" << endl;
        for (auto const& ds : golden_run.get_data_structures()) {
            cout << left << setw(20) << "\t" + to_string(ds.get_id()) << left << setw(40) << ds.get_name() << left << setw(20) << get_data_struct_type(ds.get_type()) << endl;
        }
        cout << endl;

        auto structs = golden_run.get_data_structures();
        auto max = std::max_element(structs.begin(), structs.end(), [](const DataStructure& a, const DataStructure& b) {
            return a.get_id() < b.get_id();
            });
        int max_id = max->get_id();

        while (true) {
            cout << "Conf1 -) Insert the Id of the data structure to inject (0 - " << max_id << "): ";
            cin >> conf.struct_id;
            if (conf.struct_id >= 0 && conf.struct_id <= max_id) {
                break;
            }
            else {
                cerr << "Invalid option. Try again" << endl;
            }
        }

        while (true) {
            cout << "Conf2 -) How many injection do you want to try? ";
            cin >> conf.inject_n;
            if (conf.inject_n > 0) {
                break;
            }
            else {
                cerr << "The number of injections must be greater than 0. Try again." << endl;
            }
        }

        while (true) {
            cout << "Conf3 -) Do you want to parallelize the injections? [Y/N] ";
            cin >> parallelize_str;
            std::for_each(parallelize_str.begin(), parallelize_str.end(), [](char& c) {
                c = ::toupper(c);
                });
            if (parallelize_str == "Y") {
                conf.parallelize = true;
                break;
            }
            else if (parallelize_str == "N") {
                conf.parallelize = false;
                break;
            }
            else
                cerr << "Invalid option. Try again." << endl;
        }

        while (true) {
            cout << "Conf4 -) Indicate the maximum time (in milliseconds) in which the random injection has to be performed: ";
            cin >> conf.max_time_ms;
            if (conf.max_time_ms > 0) {
                break;
            }
            else {
                cerr << "The time can't be zero or less. Try again." << endl;
            }
        }

        while (true) {
            cout << "Conf5 -) In the case of a Silence Data Corruption, do you want to search for a specific error pattern? [Y/N] ";
            cin >> pattern_error_str;
            std::for_each(pattern_error_str.begin(), pattern_error_str.end(), [](char& c) {
                c = ::toupper(c);
                });
            if (pattern_error_str == "Y") {
                cout << "\tPlease, insert the error pattern to match: ";
                cin >> conf.error_pattern;
                break;
            }
            else if (pattern_error_str == "N") {
                conf.error_pattern = "";
                break;
            }
            else
                cerr << "Invalid option. Try again." << endl;

        }

        cout << "-- Configuration completed --" << endl;
        cout << endl;
        return;
    }
}

void injection(InjectConf& conf) {
    SimulatorRun sr;
    std::error_code ec;
    SimulatorError se;

    // Spawn a simulator instance to be injected and load its data structures
    sr.init(sim_path);

    // Retrieve the data structure to be injected
    DataStructure ds = sr.get_ds_by_id(conf.struct_id);
    Injection inj(&sr, ds, conf.max_time_ms);

    // Signal to the simulator instance that it can start the scheduler
    sr.start();
    inj.init();
    inj.inject(sr.get_begin_time());
    inj.close();

    // Wait for the simulator to finish and log
    if (sr.wait_for(golden_run.duration() * DEADLOCK_TIME_FACTOR, ec)) {
        // The child exited and the timer has not expired yet
        int native_exit_code = sr.get_native_exit_code();

        if (native_exit_code) {
            se = CRASH;
        }
        else {
            // Child process exited with code 0 and everything should be ok
            sr.save_output(nullptr);

            // Perform comparison
            se = sr.compare_with_golden(golden_run, conf.error_pattern);
        }
    }
    else {
        // The child didn't exit and the timer has expired (possible deadlock)
        sr.terminate();
        se = HANG;
    }

    // Log injection results
    log_injection_trial(golden_run, sr, inj, ec, se, conf.error_pattern);
}

void sequential_injections(InjectConf& conf) {
    for (int i = 0; i < conf.inject_n; i++) {
        LOG_F(INFO, "Injection Try #%d / %d ...", i + 1, conf.inject_n);

        injection(conf);

        LOG_F(INFO, "Injection finished.", i + 1);
        LOG_F(INFO, "----------------------\n");
    }
}

void parallel_injections(InjectConf& conf, char *exe_name) {
    std::vector<bp::child> childs(conf.inject_n);
    std::cout << "Performing " << conf.inject_n << " parallel injection trials.." << std::endl;
    // Start #Injections fault injector processes (in parallel mode)
    for (int i = 0; i < conf.inject_n; i++) {
        int rand_seed = rand() % RAND_MAX;
        if (conf.error_pattern == "") {
            bp::child c(
                bp::search_path(exe_name),
                std::to_string(golden_run.get_pid()),
                std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(golden_run.duration()).count()),
                std::to_string(rand_seed),
                std::to_string(conf.struct_id),
                std::to_string(i),
                std::to_string(conf.max_time_ms),
                bp::std_out > bp::null,
                bp::std_err > bp::null
            );
            childs[i] = std::move(c);
        }
        else {
            bp::child c(
                bp::search_path(exe_name),
                std::to_string(golden_run.get_pid()),
                std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(golden_run.duration()).count()),
                std::to_string(rand_seed),
                std::to_string(conf.struct_id),
                std::to_string(i),
                std::to_string(conf.max_time_ms),
                conf.error_pattern,
                bp::std_out > bp::null,
                bp::std_err > bp::null
            );
            childs[i] = std::move(c);
        }
    }

    // Wait for the completion of all the fault injectors and join their output to the main logging file
    for (int i = 0; i < conf.inject_n; i++) {
        childs[i].wait();

        LOG_F(INFO, "Injection Try #%d / %d ...", i + 1, conf.inject_n);
        log_join(std::to_string(childs[i].id()));
        LOG_F(INFO, "Injection finished.", i + 1);
        LOG_F(INFO, "----------------------\n");
    }
}
