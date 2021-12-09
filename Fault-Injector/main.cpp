#include <iostream>
#include <string>
#include <chrono>
#include <thread>

#include "SimulatorRun.h"

int main()
{
    using namespace std::this_thread; // sleep_for, sleep_until
    using namespace std::chrono; // nanoseconds, system_clock, seconds

    //std::string sim_path = "C:/Users/rugge/Documents/development/freertos_fault_injector/project_repo/build/Win32-Debug-Simulator-All-Tasks/FreeRTOS/Simulator/FreeRTOS_Simulator";
    std::string sim_path = "D:/development/freertos_fault_injector/project_repo/build/Win32-Debug-Simulator-All-Tasks/FreeRTOS/Simulator/FreeRTOS_Simulator";
    //std::string sim_path = "cmake --version";

    SimulatorRun sr;
    std::cout << "Starting " << sim_path << " process" << std::endl;
    sr.start(sim_path);

    std::cout << "Waiting process to terminate.." << std::endl;
    sr.wait();
    std::cout << "Child process terminated" << std::endl;
    sr.show();
}