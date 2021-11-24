cmake_minimum_required(VERSION 3.10)

# set the project name
project(FreeRTOS_FaultInjector)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Sources for the kernel
file(GLOB FREERTOS_SOURCES
        "${PROJECT_SOURCE_DIR}/FreeRTOS/FreeRTOS/Source/*.c"
        "${PROJECT_SOURCE_DIR}/FreeRTOS/FreeRTOS/Source/portable/Common/*.c"
        "${PROJECT_SOURCE_DIR}/FreeRTOS/FreeRTOS/Source/portable/MemMang/heap_3.c"
        "${PROJECT_SOURCE_DIR}/FreeRTOS/FreeRTOS/Source/portable/ThirdParty/GCC/Posix/utils/wait_for_event.c"
        "${PROJECT_SOURCE_DIR}/FreeRTOS/FreeRTOS/Source/portable/ThirdParty/GCC/Posix/port.c"
        "${PROJECT_SOURCE_DIR}/FreeRTOS/FreeRTOS-Plus/Source/FreeRTOS-Plus-Trace/*.c"
        "${PROJECT_SOURCE_DIR}/FreeRTOS/FreeRTOS-Plus/Source/FreeRTOS-Plus-Trace/streamports/File/trcStreamingPort.c"
        )

# Sources for the demo
file(GLOB DEMO_SOURCES
        "${PROJECT_SOURCE_DIR}/FreeRTOS/FreeRTOS/Demo/Common/Minimal/*.c")

# add the executable
add_executable(Demo
        ${FREERTOS_SOURCES}
        ${DEMO_SOURCES})
#add_executable(FreeRTOS_Simulator tutorial.cxx)

add_definitions(-DprojCOVERAGE_TEST=0)
add_definitions(-DUSER_DEMO=0)

add_compile_options(-fsanitize=address)
add_compile_options(-fsanitize=alignment)

target_link_libraries(Demo PRIVATE Threads::Threads)

# Includes for the demo
target_include_directories(Demo PUBLIC
        "${PROJECT_SOURCE_DIR}/FreeRTOS/FreeRTOS/Source/include"
        "${PROJECT_SOURCE_DIR}/FreeRTOS/FreeRTOS/Source/portable/ThirdParty/GCC/Posix"
        "${PROJECT_SOURCE_DIR}/FreeRTOS/FreeRTOS/Source/portable/ThirdParty/GCC/Posix/utils"
        "${PROJECT_SOURCE_DIR}/FreeRTOS/FreeRTOS/Demo/Common/include"
        "${PROJECT_SOURCE_DIR}/FreeRTOS/FreeRTOS/Demo/ThirdParty/Template"
        "${PROJECT_SOURCE_DIR}/FreeRTOS/FreeRTOS/Demo/Posix_GCC"
        "${PROJECT_SOURCE_DIR}/FreeRTOS/FreeRTOS-Plus/Source/FreeRTOS-Plus-Trace/Include"
        )

