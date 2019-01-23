#ifndef CLOX_COMPILER
#define CLOX_COMPILER

#include "vm/chunk.h"
#include "vm/value.h"

int compile(const char* code, Chunk* chunk);

VmString* vmstring_take(char* chars, int length);
VmString* vmstring_copy(const char* chars, int length);

#endif
