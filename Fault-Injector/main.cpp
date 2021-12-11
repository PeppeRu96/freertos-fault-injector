#include <iostream>
#include <string>

#include "SimulatorRun.h"

#define SIMULATOR_FOLDER_PATH "C:/Users/rugge/Documents/development/freertos_fault_injector/project_repo/build/Win32-Debug-Simulator-All-Tasks/FreeRTOS/Simulator/";
#define SIMULATOR_EXE_NAME "FreeRTOS_Simulator";


std::string sim_folder_path = SIMULATOR_FOLDER_PATH;
std::string sim_exe_name = SIMULATOR_EXE_NAME;
std::string sim_path = sim_folder_path + sim_exe_name;

int main()
{

    // Start a simulator and save the golden execution
    std::cout << "Executing the simulator and saving the golden execution..." << std::endl;
    SimulatorRun golden_run;
    golden_run.start(sim_path);
    golden_run.wait();

    std::cout << "Golden execution output:" << std::endl;
    golden_run.show_output();


    // Display user menu

    // Perform selected operation
}
