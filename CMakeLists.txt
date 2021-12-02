cmake_minimum_required(VERSION 3.10)

project(FreeRTOS_FaultInjector)

set(FREERTOS_DIR ${PROJECT_SOURCE_DIR}/FreeRTOS/FreeRTOS)
set(FREERTOS_PLUS_DIR ${PROJECT_SOURCE_DIR}/FreeRTOS/FreeRTOS-Plus)

if (WIN32)
    message("Platform WIN32")
elseif(UNIX)
    if (APPLE)
        message("Platform Mac OS X")
    else()
        message("Platform Linux")
    endif()
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
