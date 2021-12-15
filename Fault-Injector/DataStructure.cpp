#include "DataStructure.h"

DataStructure::DataStructure(int id, const char* name, int type, void* address) {
	this->id = id;
	this->name = name;
	this->type = type;
	this->address = address;
}

std::ostream& operator<<(std::ostream& output, const DataStructure& ds) {
	output << "Id: " << ds.id << ", Name: " << ds.name << ", Type: " << ds.type << ", Address: " << std::hex << ds.address;
	return output;
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