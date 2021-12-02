#include <iostream>
#include <string>
#include <chrono>
#include <thread>

#include "SimulatorRun.h"

int main()
{
    using namespace std::this_thread; // sleep_for, sleep_until
    using namespace std::chrono; // nanoseconds, system_clock, seconds

    std::string sim_path = "/home/ruggeri/development/freertos_fault_injector/repo_project/cmake-build-debug/FreeRTOS/Simulator/FreeRTOS_Simulator";
    SimulatorRun sr;


    sr.start(sim_path);
    sleep_for(seconds(10));
    sr.terminate();

    sr.wait();
    sr.show();
}