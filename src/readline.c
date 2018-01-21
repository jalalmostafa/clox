#include "global.h"
#include "mem.h"
#include <stdio.h>
#include <stdlib.h>

char* readLine(const char* prompt)
{
	int size = sizeof(char) * LINEBUFSIZE;
    char* line = alloc(size);
	memset(line, 0, size);
    printf("%s", prompt);
    gets(line);
    return line;
}