#include <sync.h>

#include <iostream>
#include <string>

#include <boost/interprocess/detail/os_thread_functions.hpp>
#include <boost/interprocess/sync/named_semaphore.hpp>

namespace bi = boost::interprocess;

void signal_memory_log_finished() {
	std::string pid = std::to_string(boost::interprocess::ipcdetail::get_current_process_id());
	std::string sem_name = "binary_sem_log_struct_" + pid + "_1";
	bi::named_semaphore s(bi::open_or_create, sem_name.c_str(), 0);
	s.post();
}

void wait_before_start() {
	std::string pid = std::to_string(boost::interprocess::ipcdetail::get_current_process_id());
	std::string sem_name = "binary_sem_log_struct_" + pid + "_2";
	bi::named_semaphore s(bi::open_or_create, sem_name.c_str(), 0);
	s.wait();
}