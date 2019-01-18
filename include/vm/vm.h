#ifndef CLOX_VM
#define CLOX_VM

#include "vm/chunk.h"

typedef struct vm {
    Chunk* chunk;
    Byte* ip;
} VM;

typedef enum vm_interpret_result {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} VmInterpretResult;

void vm_init();
void vm_free();
VmInterpretResult vm_interpret(Chunk* chunk);

#endif