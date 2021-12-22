#include "DataStructure.h"

DataStructure::DataStructure(int id, const char* name, int type, void* address) {
	this->id = id;
	this->name = name;
	this->type = type;
	this->address = address;
	this->fixed_size = get_fixed_sizeof_struct(type);
}

std::ostream& operator<<(std::ostream& output, const DataStructure& ds) {
	output << "Id: " << ds.id << ", Name: " << ds.name << ", Type: " << ds.type << ", Address: " << std::hex << ds.address;
	return output;
}

size_t DataStructure::get_fixed_size() const {
	return this->fixed_size;
}

int DataStructure::get_id() const {
	return this->id;
}
std::string DataStructure::get_name() const {
	return this->name;
}
int DataStructure::get_type() const {
	return this->type;
}
void* DataStructure::get_address() const {
	return this->address;
}