# Request Threads library
find_package(Threads REQUIRED)

# Compile options and definitions
if(${DEMO} STREQUAL "BLINKY")
    message("Blinky")
    add_definitions(-DUSER_DEMO=0)
elseif(${DEMO} STREQUAL "FULL")
    message("Full")
    add_definitions(-DUSER_DEMO=1)
endif()
add_compile_options(-DprojCOVERAGE_TEST=0)

set(DEMO_INCLUDES
        ${FREERTOS_DIR}/Demo/Posix_GCC
        ${FREERTOS_DIR}/Demo/Common/include
        )

include_directories(${KERNEL_INCLUDES})
include_directories(${DEMO_INCLUDES})

file(GLOB DEMO_ONLY_SOURCES
        "${FREERTOS_DIR}/Demo/Posix_GCC/*.c"
        "${FREERTOS_DIR}/Source/portable/MemMang/heap_3.c"
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

set(DEMO_SOURCES
        ${FREERTOS_SOURCES}
        ${DEMO_ONLY_SOURCES}
        ${FREERTOS_TRACE_SOURCES}
        )

# Targets
add_executable(Demo ${DEMO_SOURCES})

# Link libraries to targets
target_link_libraries(Demo PRIVATE Threads::Threads)