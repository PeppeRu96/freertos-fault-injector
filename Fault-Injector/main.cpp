#include <iostream>
#include <iomanip>
#include <string>
#include <time.h>

#include "SimulatorRun.h"
#include "Injection.h"
#include "simulator_config.h"

//#define SIMULATOR_FOLDER_PATH "C:/Users/rugge/Documents/development/freertos_fault_injector/project_repo/build/Win32-Debug-Simulator-All-Tasks/FreeRTOS/Simulator/";
//#define SIMULATOR_FOLDER_PATH "D:/development/freertos_fault_injector/project_repo/build/Win32-Debug-Simulator-All-Tasks/FreeRTOS/Simulator/"
//#define SIMULATOR_FOLDER_PATH "/home/ruggeri/development/freertos_fault_injector/project_repo/cmake-build-debug/FreeRTOS/Simulator/"
#define SIMULATOR_FOLDER_PATH "/Users/ruggeri/development/freertos_fault_injector/project_repo/cmake-build-debug/FreeRTOS/Simulator/"
#define SIMULATOR_EXE_NAME "FreeRTOS_Simulator";


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

int main()
{
    std::cout << "######### FreeRTOS FaultInjector v" << PROJECT_VER << " #########" << std::endl;
    std::cout << std::endl;

    srand((unsigned)time(NULL));

    // Start a simulator and save the golden execution
    std::cout << "Executing the simulator and saving the golden execution..." << std::endl;

    golden_run.init(sim_path);
    golden_run.start();
    golden_run.wait();

    //std::cout << "Golden execution output:" << std::endl;
    //golden_run.show_output();
    golden_run.save_output();

    InjectConf conf;
    // Display user menu
    menu(conf);

    // Perform selected operation
    std::cout << "-- Injections start --" << std::endl;
    // Sequential injections
    for (int i = 0; i < conf.inject_n; i++) {
        std::cout << "Injection Try #" << i + 1 << " ..." << std::endl;
        SimulatorRun sr;
        sr.init(sim_path);
        DataStructure ds = sr.get_ds_by_id(conf.struct_id);
        Injection inj(ds, conf.max_time_ms);

        // Start
        sr.start();
        inj.inject();

        // TODO: The simulator may crash or be in deadlock! Handle these cases
        sr.wait();
        sr.save_output();
        //sr.show_output();

        // TODO: compare the output and log the errors
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

        cout << "-- Injection configuration --" << endl;
        cout << "Injectable data structures:" << endl;
        cout << left << setw(20) << "\tId" << left << setw(40) << "Name" << left << setw(20) << "Type" << endl;
        for (auto const& ds : golden_run.get_data_structures()) {
            cout << left << setw(20) << "\t" + to_string(ds.get_id()) << left << setw(40) << ds.get_name() << left << setw(20) << ds.get_type() << endl;
        }
        cout << endl;
        while (true) {
            cout << "Conf1 -) Insert the Id of the data structure to inject: ";
            cin >> conf.struct_id;
            if (conf.struct_id >= 0) {
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
