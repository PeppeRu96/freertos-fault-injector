#include <console.h>
#include <stdarg.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <boost/interprocess/detail/os_thread_functions.hpp>
#include "simulator_config.h"

#include <FreeRTOS.h>
#include <semphr.h>

SemaphoreHandle_t xStdioMutex;
StaticSemaphore_t xStdioMutexBuffer;

std::vector<std::string> output;

void console_init( void )
{
    xStdioMutex = xSemaphoreCreateMutexStatic(&xStdioMutexBuffer);
}

void console_print(const char* fmt,
    ...) {
    // TODO: protect with a mutex
    va_list vargs;
    char buffer[1000];

    va_start(vargs, fmt);

    xSemaphoreTake(xStdioMutex, portMAX_DELAY);

    vprintf(fmt, vargs);
    va_start(vargs, fmt);
    vsprintf(buffer, fmt, vargs);

    std::string s = buffer;
    output.push_back(s);

    xSemaphoreGive(xStdioMutex);

    va_end(vargs);
}

void write_output_to_file(void) {
    std::ofstream out_file;
    std::string s1 = OUTPUT_FILE_PREFIX;
    std::string s2 = std::to_string(boost::interprocess::ipcdetail::get_current_process_id());
    std::string s3 = ".txt";
    std::string path = "output/" + s1 + s2 + s3;

    out_file.open(path);
    if (out_file.is_open()) {
        for (auto s : output) {
            out_file << s;
        }
    }
    else {
        std::cerr << "Unable to open " << path << " for writing the output." << std::endl;
        exit(1);
    }
    out_file.close();
}
