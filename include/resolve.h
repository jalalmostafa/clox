#ifndef RESLV_H
#define RESLV_H

#include "parse.h"

int resolve(Stmt* stmt);

typedef enum function_type_t {
    FUNCTION_TYPE_NONE,
    FUNCTION_TYPE_FUNCTION,
    FUNCTION_TYPE_METHOD,
    FUNCTION_TYPE_CTOR
} FunctionType;

typedef enum class_type_t {
    CLASS_TYPE_NONE,
    CLASS_TYPE_CLASS,
    CLASS_TYPE_SUBCLASS
} ClassType;

#define RESOLVER_SUCCESS "RESOLVER_SUCCESS"

#endif
