#include <iostream>
#include <iomanip>
#include <string>
#include <time.h>
#include <algorithm>

#include "SimulatorRun.h"
#include "Injection.h"
#include "simulator_config.h"
#include "memory_logger.h"

#include "loguru.hpp"
#include "logger.h"

// PC fisso - Giuseppe
//#define SIMULATOR_FOLDER_PATH "D:/development/freertos_fault_injector/project_repo/build/Win32-Debug-Simulator-All-Tasks/FreeRTOS/Simulator/"
//#define SIMULATOR_FOLDER_PATH "D:/development/freertos_fault_injector/project_repo/build/Win32-Debug-Simulator-Conf1/FreeRTOS/Simulator/"
#define SIMULATOR_FOLDER_PATH "D:/development/freertos_fault_injector/project_repo/build/Win32-Debug-FaultInjector/FreeRTOS/Simulator/"


// PC portatile - Giuseppe
//#define SIMULATOR_FOLDER_PATH "C:/Users/rugge/Documents/development/freertos_fault_injector/project_repo/build/Win32-Debug-Simulator-All-Tasks/FreeRTOS/Simulator/";
//#define SIMULATOR_FOLDER_PATH "C:/Users/rugge/Documents/development/freertos_fault_injector/project_repo/build/Win32-Debug-Simulator-Conf1/FreeRTOS/Simulator/";

// Ubuntu - Giuseppe
//#define SIMULATOR_FOLDER_PATH "/home/ruggeri/development/freertos_fault_injector/project_repo/cmake-build-debug/FreeRTOS/Simulator/"

// Mac - Giuseppe
//#define SIMULATOR_FOLDER_PATH "/Users/ruggeri/development/freertos_fault_injector/project_repo/cmake-build-debug/FreeRTOS/Simulator/"

#define SIMULATOR_EXE_NAME      "FreeRTOS_Simulator"
#define DEADLOCK_TIME_FACTOR    2


std::string sim_folder_path = SIMULATOR_FOLDER_PATH;
std::string sim_exe_name = SIMULATOR_EXE_NAME;
std::string sim_path = sim_folder_path + sim_exe_name;

typedef struct {
    int struct_id;
    int inject_n;
    long long max_time_ms;
    bool parallelize;
} InjectConf;

void menu(InjectConf &conf);

SimulatorRun golden_run;
std::error_code golden_run_ec;

int main(int argc, char ** argv)
{
    std::cout << "######### FreeRTOS FaultInjector v" << PROJECT_VER << " #########" << std::endl;
    std::cout << std::endl;

    log_init();

    // LOG_F(INFO, "I'm hungry for some %.3f!", 3.14159);

    srand((unsigned)time(NULL));

    // Start a simulator and save the golden execution
    std::cout << "Executing the simulator and saving the golden execution..." << std::endl;

    golden_run.init(sim_path);
    //golden_run.init("C:/Users/rugge/Documents/development/freertos_fault_injector/project_repo/build/Win32-Debug-Simulator-Conf1/FreeRTOS/Simulator/FreeRTOS_Simulator_gold");
    golden_run.start();
    golden_run_ec = golden_run.wait();
    // Handle the case in which the golden execution fails for some reason (integrate the error handling)
    std::cout << "Golden execution native exit code: " << golden_run.get_native_exit_code() << std::endl;
    golden_run.save_output();
    std::cout << "Golden run execution took " << std::chrono::duration_cast<std::chrono::seconds>(golden_run.duration()).count() << " seconds." << std::endl;

    InjectConf conf;
    // Display user menu
    menu(conf);

    // Perform selected operation
    std::cout << "-- Injections start --" << std::endl;
    // Sequential injections
    for (int i = 0; i < conf.inject_n; i++) {
        std::cout << "Injection Try #" << i + 1 << " ..." << std::endl;
        SimulatorRun sr;
        std::error_code ec;

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
        inj.print_stats();

        // Wait && Log
        if (sr.wait_for(golden_run.duration() * DEADLOCK_TIME_FACTOR, ec)) {
            // The child exited and the timer has not expired yet
            int native_exit_code = sr.get_native_exit_code();

            std::cout << "Child exited and the timer has not expired yet." << std::endl;

            // Handle the error to check if the program crashed or if it is ok
            std::cout << "Child native exit code: " << native_exit_code << std::endl;

            if (native_exit_code) {
                // Child process exited with some errors (maybe crash?)
                std::cout << "Child crashed!" << std::endl;
                std::cout << "Error code: " << ec << ", native exit code: " << sr.get_native_exit_code() << std::endl;

                std::cout << "\n---------------- CRASH -----------------\n\n" << std::endl;
            }
            else {
                // Child process exited with code 0 and everything should be ok
                std::cout << "No errors. Proceed to do the the golden execution output comparison. Error code: " << ec << std::endl;
                sr.save_output();

                // Perform comparison
                SimulatorError se = sr.compare_with_golden(golden_run);
                if (se == MASKED) {
                    std::cout << "\n---------------- MASKED -----------------\n\n" << std::endl;
                }
                else if (se == SDC) {
                    std::cout << "\n---------------- Silence Data Corruption -----------------\n\n" << std::endl;
                }
                else if (se == DELAY) {
                    std::cout << "\n---------------- DELAY -----------------\n\n" << std::endl;
                }
            }
            std::cout << "Injection #" << i+1 << " run execution took " << std::chrono::duration_cast<std::chrono::seconds>(sr.duration()).count() << " seconds.\n\n" << std::endl;

        }
        else {
            // The child didn't exit and the timer has expired (possible deadlock)
            sr.terminate();
            // sr.wait() needed?
            std::cout << "Child didn't exit and the timer has expired. Possible deadlock recognized. Child process killed." << std::endl;
            std::cout << "Error code: " << ec << std::endl;
            // Classify as deadlock

            std::cout << "\n---------------- DEADLOCK -----------------\n\n" << std::endl;
        }
        //Sleep(30000);
    }
}

void menu(InjectConf &conf) {
    using namespace std;
    int op;
    string parallelize_str;

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
        cout << "-- Configuration completed --" << endl;
        cout << endl;
        return;
    }
}
