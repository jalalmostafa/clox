
#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include "parse.h"

void except(const char* format, ...);

void parse_error(Token* tkn, const char* msg, ...);

#endif
