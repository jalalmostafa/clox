#ifndef CLOX_COMPILER
#define CLOX_COMPILER

#include "vm/chunk.h"
#include "vm/value.h"

VmFunction* compile(const char* code);

VmString* vmstring_take(char* chars, size_t length);
VmString* vmstring_copy(const char* chars, size_t length);
VmFunction* vmfunction_new();
VmNative* vmnative_new(NativeFn function);

#endif
