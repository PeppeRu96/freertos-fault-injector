#include "structuresLog.h"
#include <stdio.h>
#include <stdlib.h>

FILE* structuresLog;
int nextID;

void log_struct(char* name, int type, void* address){
    fprintf(structuresLog, "%d %s %d %p\n", nextID++, name, type, address);
}

void log_start(){
    structuresLog = fopen("structures.txt", "w");
    fprintf(structuresLog, "ID Name Type Address\n");
}

void log_end(){
    fclose(structuresLog);
}
