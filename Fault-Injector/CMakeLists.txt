
configure_file(${SIMULATOR_DIR}/simulator_config.h.in simulator_config.h)

# Includes
set(FI_INCLUDES
		${FAULT_INJECTOR_DIR}
		${CMAKE_CURRENT_BINARY_DIR}
		${SIMULATOR_DIR}
		)

# Sources
file(GLOB FI_SOURCES
		"${FAULT_INJECTOR_DIR}/*.cpp"
		"${FAULT_INJECTOR_DIR}/*.hpp"
)

file(GLOB FI_SOURCES
		${FI_SOURCES}
		"${FAULT_INJECTOR_DIR}/*.c"
)

# Heap management (platform-dependent)
if (WIN32)
    file(GLOB FI_SOURCES
            ${FI_SOURCES}
            "${FREERTOS_DIR}/Source/portable/MemMang/heap_4.c"
            )
elseif(UNIX)
    file(GLOB FI_SOURCES
            ${FI_SOURCES}
            "${FREERTOS_DIR}/Source/portable/MemMang/heap_3.c"
            )
endif()

file(GLOB FI_SOURCES
		${FI_SOURCES}
		"${SIMULATOR_DIR}/memory_logger.cpp"
)

set(SOURCES
		${FI_SOURCES}
        ${FREERTOS_SOURCES}
)

add_executable(FreeRTOS_FaultInjector ${SOURCES})

target_include_directories(FreeRTOS_FaultInjector PRIVATE ${FI_INCLUDES})

target_link_libraries(FreeRTOS_FaultInjector PRIVATE Threads::Threads)
if (UNIX)
	target_link_libraries(FreeRTOS_FaultInjector PRIVATE rt)
    target_link_libraries(FreeRTOS_FaultInjector PRIVATE dl)
endif()

if (${Boost_FOUND})
	# Include header-only boost
	target_include_directories(FreeRTOS_FaultInjector PRIVATE ${Boost_INCLUDE_DIRS})
	target_link_directories(FreeRTOS_FaultInjector PRIVATE ${Boost_LIBRARY_DIRS})
	target_link_libraries(FreeRTOS_FaultInjector PRIVATE ${Boost_LIBRARIES})
endif()


