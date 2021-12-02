# Hints to FindBoost
# Path to the boost root (it must be installed on your system)
set(BOOST_ROOT "/usr/local/boost_1_77_0" CACHE STRING "The path to the root of boost on your system")

find_package(Boost 1.77.0 REQUIRED)

add_executable(FreeRTOS_FaultInjector main.cpp SimulatorRun.cpp)

# Include header-only boost
target_include_directories(FreeRTOS_FaultInjector PRIVATE ${Boost_INCLUDE_DIRS})
