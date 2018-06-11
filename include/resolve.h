#ifndef RESLV_H
#define RESLV_H

#include "parse.h"

int resolve(Stmt* stmt);

typedef enum function_type_t {
    FUNCTION_TYPE_NONE,
    FUNCTION_TYPE_FUNCTION
} FunctionType;

#define RESOLVER_SUCCESS "RESOLVER_SUCCESS"

#endif
