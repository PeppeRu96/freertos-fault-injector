#include <console.h>
#include <stdarg.h>
#include <stdio.h>
#include <iostream>


void console_init( void )
{
    // TODO: initialize the mutex
}

void console_print(const char* fmt,
    ...) {
    // TODO: protect with a mutex
    va_list vargs;

    va_start(vargs, fmt);

    //xSemaphoreTake(xStdioMutex, portMAX_DELAY);

    vprintf(fmt, vargs);

    //xSemaphoreGive(xStdioMutex);

    va_end(vargs);
    //std::cout << s << std::endl;
}
