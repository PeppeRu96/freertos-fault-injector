cmake_minimum_required(VERSION 3.10)

project(FreeRTOS_FaultInjector LANGUAGES C CXX VERSION 1.0)

set(FREERTOS_DIR ${PROJECT_SOURCE_DIR}/FreeRTOS/FreeRTOS)
set(FREERTOS_PLUS_DIR ${PROJECT_SOURCE_DIR}/FreeRTOS/FreeRTOS-Plus)
set(SIMULATOR_DIR ${PROJECT_SOURCE_DIR}/FreeRTOS/Simulator)
set(FAULT_INJECTOR_DIR ${PROJECT_SOURCE_DIR}/Fault-Injector)

if (WIN32)
    message("Platform WIN32")
elseif(UNIX)
    if (APPLE)
        message("Platform Mac OS X")
    else()
        message("Platform Linux")
    endif()
endif()

# Boost
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
else()
	message("Boost not found! You can build the demo anyway.")
endif()

# Configuration variables and options

# Kernel includes
# Platform independent
set(KERNEL_INCLUDES
        ${FREERTOS_DIR}/Source/include
        ${FREERTOS_PLUS_DIR}/Source/FreeRTOS-Plus-Trace/Include
        )

# Platform dependent
if (WIN32)
    set(KERNEL_INCLUDES
                ${KERNEL_INCLUDES}
                ${FREERTOS_DIR}/Source/portable/MSVC-MingW
    )
elseif (UNIX)
    set(KERNEL_INCLUDES
            ${KERNEL_INCLUDES}
            ${FREERTOS_DIR}/Source/portable/ThirdParty/GCC/Posix
            ${FREERTOS_DIR}/Source/portable/ThirdParty/GCC/Posix/utils
    )
endif()

include_directories(${KERNEL_INCLUDES})

# Kernel sources
# Platform-independent
file(GLOB FREERTOS_SOURCES
        "${FREERTOS_DIR}/Source/*.c"
        )

# Platform-dependent
if (WIN32)
    file(GLOB FREERTOS_SOURCES
            ${FREERTOS_SOURCES}
            "${FREERTOS_DIR}/Source/portable/MSVC-MingW/*.c"
    )
elseif(UNIX)
    file(GLOB FREERTOS_SOURCES
                ${FREERTOS_SOURCES}
                "${FREERTOS_DIR}/Source/portable/ThirdParty/GCC/Posix/utils/wait_for_event.c"
                "${FREERTOS_DIR}/Source/portable/ThirdParty/GCC/Posix/port.c"
    )
endif()

# FreeRTOS-Plus Trace Sources
file(GLOB FREERTOS_TRACE_SOURCES
        "${FREERTOS_PLUS_DIR}/Source/FreeRTOS-Plus-Trace/trcKernelPort.c"
        "${FREERTOS_PLUS_DIR}/Source/FreeRTOS-Plus-Trace/trcSnapshotRecorder.c"
        "${FREERTOS_PLUS_DIR}/Source/FreeRTOS-Plus-Trace/trcStreamingRecorder.c"
        "${FREERTOS_PLUS_DIR}/Source/FreeRTOS-Plus-Trace/streamports/File/trcStreamingPort.c"
        )

add_subdirectory(FreeRTOS/FreeRTOS/Demo)
add_subdirectory(FreeRTOS/Simulator)
add_subdirectory(Fault-Injector)
