//
// Created by ruggeri on 14/12/21.
//

#ifndef FREERTOS_FAULTINJECTOR_DATASTRUCTURE_H
#define FREERTOS_FAULTINJECTOR_DATASTRUCTURE_H

#include "memory_logger.h"

#include <string>
#include <iostream>
#include "FreeRTOSInterface.h"

class DataStructure {
private:
	int id;
    std::string name;
	int type;
	void *address;
	size_t fixed_size;

	char struct_before[500];

public:
    DataStructure(int id, const char* name, int type, void* address);

	size_t get_fixed_size() const;
	size_t get_exploded_size() const;
	void get_next_expansion(size_t byte_number, void** byte_to_inject, void** addr_to_read, size_t* size_to_read) const;
	int get_id() const;
	std::string get_name() const;
	int get_type() const;
	void* get_address() const;
	char* get_struct_before();

    friend std::ostream& operator<<(std::ostream& output, const DataStructure& ds);
};

#endif //FREERTOS_FAULTINJECTOR_DATASTRUCTURE_H