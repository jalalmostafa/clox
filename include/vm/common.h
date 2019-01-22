#ifndef CLOX_COMMON
#define CLOX_COMMON
#include "includes/lox-config.h"
#include <limits.h>

typedef unsigned char Byte;

#define BYTE_MAX UCHAR_MAX

#ifdef DEBUG
#define DEBUG_PRINT_CODE
#define DEBUG_EXECUTION_TRACE
#endif

#endif
