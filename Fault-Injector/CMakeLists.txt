# You need to install Boost on your system (not in the project dir!)
# Typical folder under Windows: C:\local\boost
# Typical folder under Linux: /usr/local/boost
# Do not modify the paths here, you have to modify either CMakeSettings.json (under Windows) or CMakeCache.txt (under Linux/MacOS)

# Hints to FindBoost
set(BOOST_ROOT "/usr/local/boost_1_77_0" CACHE STRING "The path to the root of boost on your system")
set(BOOST_LIBRARYDIR ${BOOST_ROOT}/stage/lib32-msvc-14.2 CACHE STRING "The path to the boot libraries on your system")
set(BOOST_INCLUDEDIR ${BOOST_ROOT} CACHE STRING "The path to the include directories (commonly the same as BOOST_ROOT)")

set(Boost_USE_STATIC_LIBS OFF CACHE BOOL "Use the boost static libraries")
set(Boost_USE_MULTITHREADED ON CACHE BOOL "Use multi-threaded version of Boost")
set(Boost_USE_STATIC_RUNTIME OFF CACHE BOOL "If link boost with the static c++ runtime")
set(Boost_NO_SYSTEM_PATHS ON CACHE BOOL "If you allow or not to search for boost automatically in your system PATH (better no)")

add_executable(FreeRTOS_FaultInjector main.cpp SimulatorRun.cpp)

# Suppress policy warning under Windows
if (POLICY CMP0074)
	cmake_policy(SET CMP0074 NEW)
endif()

find_package(Boost COMPONENTS system filesystem thread date_time iostreams)

if (${Boost_FOUND})
	message("Founded Boost with version: ${Boost_VERSION}")
	message("Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
	message("Boost_LIBRARY_DIRS: ${Boost_LIBRARY_DIRS}")
	message("Boost_LIBRARIES: ${Boost_LIBRARIES}")

	# Include header-only boost
	target_include_directories(FreeRTOS_FaultInjector PRIVATE ${Boost_INCLUDE_DIRS})
	target_link_directories(FreeRTOS_FaultInjector PRIVATE ${Boost_LIBRARY_DIRS})
else()
	message("Boost not found! You will not be able to build the fault injector.")
endif()


