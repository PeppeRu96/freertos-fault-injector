cmake_minimum_required(VERSION 3.10)

project(FreeRTOS_FaultInjector)

set(FREERTOS_DIR ${PROJECT_SOURCE_DIR}/FreeRTOS/FreeRTOS)
set(FREERTOS_PLUS_DIR ${PROJECT_SOURCE_DIR}/FreeRTOS/FreeRTOS-Plus)

set(DEMO "BLINKY" CACHE STRING "")

# Kernel includes
set(KERNEL_INCLUDES
        ${FREERTOS_DIR}/Source/include
        ${FREERTOS_DIR}/Source/portable/ThirdParty/GCC/Posix
        ${FREERTOS_DIR}/Source/portable/ThirdParty/GCC/Posix/utils
        ${FREERTOS_PLUS_DIR}/Source/FreeRTOS-Plus-Trace/Include
        )

# Kernel sources
file(GLOB FREERTOS_SOURCES
        "${FREERTOS_DIR}/Source/*.c"
        "${FREERTOS_DIR}/Source/portable/ThirdParty/GCC/Posix/utils/wait_for_event.c"
        "${FREERTOS_DIR}/Source/portable/ThirdParty/GCC/Posix/port.c"
        )

# FreeRTOS-Plus Trace Sources
file(GLOB FREERTOS_TRACE_SOURCES
        "${FREERTOS_PLUS_DIR}/Source/FreeRTOS-Plus-Trace/trcKernelPort.c"
        "${FREERTOS_PLUS_DIR}/Source/FreeRTOS-Plus-Trace/trcSnapshotRecorder.c"
        "${FREERTOS_PLUS_DIR}/Source/FreeRTOS-Plus-Trace/trcStreamingRecorder.c"
        "${FREERTOS_PLUS_DIR}/Source/FreeRTOS-Plus-Trace/streamports/File/trcStreamingPort.c"
        )

add_subdirectory(FreeRTOS/FreeRTOS/Demo)
