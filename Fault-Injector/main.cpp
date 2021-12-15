#include <iostream>
#include <iomanip>
#include <string>

#include "SimulatorRun.h"

//#define SIMULATOR_FOLDER_PATH "C:/Users/rugge/Documents/development/freertos_fault_injector/project_repo/build/Win32-Debug-Simulator-All-Tasks/FreeRTOS/Simulator/";
#define SIMULATOR_FOLDER_PATH "D:/development/freertos_fault_injector/project_repo/build/Win32-Debug-Simulator-All-Tasks/FreeRTOS/Simulator/"
#define SIMULATOR_EXE_NAME "FreeRTOS_Simulator";


std::string sim_folder_path = SIMULATOR_FOLDER_PATH;
std::string sim_exe_name = SIMULATOR_EXE_NAME;
std::string sim_path = sim_folder_path + sim_exe_name;

void menu();

SimulatorRun golden_run;

int main()
{
    // Start a simulator and save the golden execution
    std::cout << "Executing the simulator and saving the golden execution..." << std::endl;

    golden_run.start(sim_path);
    golden_run.wait();

    //std::cout << "Golden execution output:" << std::endl;
    //golden_run.show_output();
    golden_run.save_output();

    // Display user menu
    menu();

    // Perform selected operation
}

void menu() {
    using namespace std;

    int inject_ds_id;
    int inject_n;
    int max_time_secs;
    string parallelize_str;

    cout << "######### FreeRTOS FaultInjector #########" << endl;
    cout << "-- Injection configuration --" << endl;
    cout << "Injectable data structures:" << endl;
    cout << left << setw(20) << "\tId" << left << setw(40) << "Name" << left << setw(20) << "Type" << endl;
    for (auto const& ds : golden_run.get_data_structures()) {
        cout << left << setw(20) << "\t" + to_string(ds.get_id()) << left << setw(40) << ds.get_name() << left << setw(20) << ds.get_type() << endl;
    }
    cout << endl;
    cout << "Conf1 -) Insert the Id of the data structure to inject: ";
    cin >> inject_ds_id;

    cout << "Conf2 -) How many injection do you want to try? ";
    cin >> inject_n;

    cout << "Conf3 -) Do you want to parallelize the injections? [Y/N] ";
    cin >> parallelize_str;

    cout << "Conf4 -) Indicate the maximum time (in seconds) in which the random injection has to be performed: ";
    cin >> max_time_secs;
}
