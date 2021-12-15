# ---- DEMO ----

option(BLINKY_DEMO "Build a blinky demo only rather than the full one" ON)

configure_file(${FREERTOS_DIR}/Demo/demo_config.h.in demo_config.h)
configure_file(${SIMULATOR_DIR}/simulator_config.h.in simulator_config.h)

# Request Threads library
find_package(Threads REQUIRED)

# Includes
# Platform dependent include directories
if (WIN32)
    set(DEMO_INCLUDES
            ${FREERTOS_DIR}/Demo/WIN32-MSVC
            ${FREERTOS_DIR}/Demo/WIN32-MSVC/Trace_Recorder_Configuration
    )
elseif (UNIX)
    set(DEMO_INCLUDES
            ${FREERTOS_DIR}/Demo/Posix_GCC
    )
endif()

# Platform independent include directories
set(DEMO_INCLUDES
        ${DEMO_INCLUDES}
        ${FREERTOS_DIR}/Demo/Common/include
        ${CMAKE_CURRENT_BINARY_DIR}
        ${SIMULATOR_DIR}
)

# Sources
# Platform-independent
file(GLOB DEMO_ONLY_SOURCES
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
        "${SIMULATOR_DIR}/memory_logger.cpp"
        )

# Platform-dependent
if (WIN32)
    file(GLOB DEMO_ONLY_SOURCES
            ${DEMO_ONLY_SOURCES}
            "${FREERTOS_DIR}/Demo/WIN32-MSVC/*.c"
            "${FREERTOS_DIR}/Source/portable/MemMang/heap_5.c"
            "${FREERTOS_DIR}/Demo/Common/Minimal/TaskNotifyArray.c"
        )
elseif(UNIX)
    file(GLOB DEMO_ONLY_SOURCES
            ${DEMO_ONLY_SOURCES}
            "${FREERTOS_DIR}/Demo/Posix_GCC/*.c"
            "${FREERTOS_DIR}/Source/portable/MemMang/heap_3.c"
    )
endif()

set(DEMO_SOURCES
        ${FREERTOS_SOURCES}
        ${DEMO_ONLY_SOURCES}
        ${FREERTOS_TRACE_SOURCES}
        )

# Targets
add_executable(Demo ${DEMO_SOURCES})

target_include_directories(Demo PRIVATE ${DEMO_INCLUDES})

# Compile options and definitions
if (UNIX)
    if(BLINKY_DEMO)
        message("Blinky Demo")
        target_compile_definitions(Demo PRIVATE USER_DEMO=0)
    else()
        message("Full Demo")
        target_compile_definitions(Demo PRIVATE USER_DEMO=1)
    endif()
    target_compile_definitions(Demo PRIVATE projCOVERAGE_TEST=0)
endif()

# Link libraries to targets
target_link_libraries(Demo PRIVATE Threads::Threads)

# Link Boost
if (${Boost_FOUND})
    # Include header-only boost
    target_include_directories(Demo PRIVATE ${Boost_INCLUDE_DIRS})
    target_link_directories(Demo PRIVATE ${Boost_LIBRARY_DIRS})
endif()