#include "Injection.h"

Injection::Injection(DataStructure ds, long long max_time_ms) : ds(ds) {
	this->max_time_ms = max_time_ms;

	this->random_time_ms = rand() % max_time_ms;
}

void Injection::inject() {

}
