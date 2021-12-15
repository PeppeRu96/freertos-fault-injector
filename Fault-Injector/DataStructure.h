//
// Created by ruggeri on 14/12/21.
//

#ifndef FREERTOS_FAULTINJECTOR_DATASTRUCTURE_H
#define FREERTOS_FAULTINJECTOR_DATASTRUCTURE_H

#include "memory_logger.h"

#include <string>
#include <iostream>

class DataStructure {
private:
	int id;
    std::string name;
	int type;
	void *address;
public:
    DataStructure(int id, const char* name, int type, void* address);

	int get_id() const;
	std::string get_name() const;
	int get_type() const;
	void* get_address() const;

    friend std::ostream& operator<<(std::ostream& output, const DataStructure& ds);
};

#endif //FREERTOS_FAULTINJECTOR_DATASTRUCTURE_H