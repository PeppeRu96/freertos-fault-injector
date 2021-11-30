# ---- FreeRTOS Simulator ----

set(SIMULATOR_DIR ${PROJECT_SOURCE_DIR}/FreeRTOS/Simulator)

# Request Threads library
find_package(Threads REQUIRED)

# Includes
set(SIMULATOR_INCLUDES
        ${SIMULATOR_DIR}
)

# Demo and configuration headers
set(SIMULATOR_INCLUDES
        ${SIMULATOR_INCLUDES}
        ${FREERTOS_DIR}/Demo/Common/include
        ${CMAKE_CURRENT_BINARY_DIR}
)

# Sources
# Simulator sources
file(GLOB SIMULATOR_SOURCES
        "${SIMULATOR_DIR}/*.c"
        "${SIMULATOR_DIR}/*.cpp"
        )

# Heap management (platform-dependent)
if (WIN32)
    file(GLOB SIMULATOR_SOURCES
            ${SIMULATOR_SOURCES}
            "${FREERTOS_DIR}/Source/portable/MemMang/heap_4.c"
            )
elseif(UNIX)
    file(GLOB SIMULATOR_SOURCES
            ${SIMULATOR_SOURCES}
            "${FREERTOS_DIR}/Source/portable/MemMang/heap_3.c"
            )
endif()

# Demo tasks sources
file(GLOB SIMULATOR_SOURCES
        ${SIMULATOR_SOURCES}
        "${FREERTOS_DIR}/Demo/Common/Minimal/AbortDelay.c"
        "${FREERTOS_DIR}/Demo/Common/Minimal/BlockQ.c"
        "${FREERTOS_DIR}/Demo/Common/Minimal/blocktim.c"
        "${FREERTOS_DIR}/Demo/Common/Minimal/countsem.c"
        "${FREERTOS_DIR}/Demo/Common/Minimal/death.c"
        "${FREERTOS_DIR}/Demo/Common/Minimal/dynamic.c"
        "${FREERTOS_DIR}/Demo/Common/Minimal/EventGroupsDemo.c"
        "${FREERTOS_DIR}/Demo/Common/Minimal/flop.c"
        "${FREERTOS_DIR}/Demo/Common/Minimal/GenQTest.c"
        "${FREERTOS_DIR}/Demo/Common/Minimal/integer.c"
        "${FREERTOS_DIR}/Demo/Common/Minimal/IntSemTest.c"
        "${FREERTOS_DIR}/Demo/Common/Minimal/MessageBufferAMP.c"
        "${FREERTOS_DIR}/Demo/Common/Minimal/MessageBufferDemo.c"
        "${FREERTOS_DIR}/Demo/Common/Minimal/PollQ.c"
        "${FREERTOS_DIR}/Demo/Common/Minimal/QPeek.c"
        "${FREERTOS_DIR}/Demo/Common/Minimal/QueueOverwrite.c"
        "${FREERTOS_DIR}/Demo/Common/Minimal/QueueSet.c"
        "${FREERTOS_DIR}/Demo/Common/Minimal/QueueSetPolling.c"
        "${FREERTOS_DIR}/Demo/Common/Minimal/recmutex.c"
        "${FREERTOS_DIR}/Demo/Common/Minimal/semtest.c"
        "${FREERTOS_DIR}/Demo/Common/Minimal/StaticAllocation.c"
        "${FREERTOS_DIR}/Demo/Common/Minimal/StreamBufferDemo.c"
        "${FREERTOS_DIR}/Demo/Common/Minimal/StreamBufferInterrupt.c"
        "${FREERTOS_DIR}/Demo/Common/Minimal/TaskNotify.c"
        "${FREERTOS_DIR}/Demo/Common/Minimal/TimerDemo.c"
        )

set(SOURCES
        ${FREERTOS_SOURCES}
        ${SIMULATOR_SOURCES}
        )

# Targets
add_executable(FreeRTOS_Simulator ${SOURCES})

target_include_directories(FreeRTOS_Simulator PRIVATE ${SIMULATOR_INCLUDES})

# Link libraries to targets
target_link_libraries(FreeRTOS_Simulator PRIVATE Threads::Threads)