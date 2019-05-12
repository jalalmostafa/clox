#ifndef CLOX_COMMON
#define CLOX_COMMON
#include "includes/lox-config.h"
#include <limits.h>

typedef unsigned char Byte;
typedef unsigned int Hash;

#define BYTE_MAX UCHAR_MAX
#define BYTE_COUNT (BYTE_MAX + 1)

#ifdef DEBUG
#define DEBUG_PRINT_CODE
#define DEBUG_EXECUTION_TRACE
#endif

#endif
