#include <stdlib.h>
#include <stdio.h>

#ifndef INCLUDE_STRUCTURES_LOG_H
#define INCLUDE_STRUCTURES_LOG_H


void log_struct(char* name, char* type, void* address);
void log_start();
void log_end();

#endif
