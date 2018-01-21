#include "global.h"
#include "mem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* readLine(const char* prompt)
{
    int size = sizeof(char) * LINEBUFSIZE;
    char* line = (char*)alloc(size);
    memset((void*)line, 0, size);
    printf("%s", prompt);
    fgets(line, size, stdin);
    return line;
}