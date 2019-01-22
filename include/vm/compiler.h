#ifndef CLOX_COMPILER
#define CLOX_COMPILER

#include "vm/chunk.h"

int compile(const char* code, Chunk* chunk);

#endif
