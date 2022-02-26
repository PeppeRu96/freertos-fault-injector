#include "DataStructure.h"

#include "memory_logger.h"

DataStructure::DataStructure(int id, const char* name, int type, void* address) {
	this->id = id;
	this->name = name;
	this->type = type;
	this->address = address;
	this->fixed_size = get_fixed_sizeof_struct(type);
}

std::ostream& operator<<(std::ostream& output, const DataStructure& ds) {
	output << "Id: " << ds.id << ", Name: " << ds.name << ", Type: " << get_data_struct_type(ds.type) << ", Address: " << std::hex << ds.address;
	return output;
}

size_t DataStructure::get_fixed_size() const {
	return this->fixed_size;
}

size_t DataStructure::get_exploded_size() const {
	return get_exploded_sizeof_struct(type, (void*)struct_before);
}

void DataStructure::get_next_expansion(size_t byte_number, void** byte_to_inject, void** addr_to_read, size_t* size_to_read) const {
	get_next_expansion_struct(type, (void*)struct_before, byte_number, byte_to_inject, addr_to_read, size_to_read);
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

char* DataStructure::get_struct_before() {
	return this->struct_before;
}