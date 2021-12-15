
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
)

file(GLOB FI_SOURCES
		${FI_SOURCES}
		"${SIMULATOR_DIR}/memory_logger.cpp"
)

add_executable(FreeRTOS_FaultInjector ${FI_SOURCES})

target_include_directories(FreeRTOS_FaultInjector PRIVATE ${FI_INCLUDES})

target_link_libraries(FreeRTOS_FaultInjector PRIVATE Threads::Threads)

if (${Boost_FOUND})
	# Include header-only boost
	target_include_directories(FreeRTOS_FaultInjector PRIVATE ${Boost_INCLUDE_DIRS})
	target_link_directories(FreeRTOS_FaultInjector PRIVATE ${Boost_LIBRARY_DIRS})
endif()


