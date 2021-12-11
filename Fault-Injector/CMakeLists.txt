
configure_file(${SIMULATOR_DIR}/simulator_config.h.in simulator_config.h)

set(FI_INCLUDES
		${FAULT_INJECTOR_DIR}
		${CMAKE_CURRENT_BINARY_DIR}
		)

add_executable(FreeRTOS_FaultInjector main.cpp SimulatorRun.cpp)

target_include_directories(FreeRTOS_FaultInjector PRIVATE ${FI_INCLUDES})

if (${Boost_FOUND})
	# Include header-only boost
	target_include_directories(FreeRTOS_FaultInjector PRIVATE ${Boost_INCLUDE_DIRS})
	target_link_directories(FreeRTOS_FaultInjector PRIVATE ${Boost_LIBRARY_DIRS})
endif()


