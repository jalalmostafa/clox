#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void except(const char* format, ...)
{
    va_list list;
    va_start(list, format);
    vfprintf(stderr, format, list);
    va_end(list);
    // if (!interactive) {
    //     exit(EXIT_FAILURE);
    // }
}
