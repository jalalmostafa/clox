#ifndef CLOX_VM
#define CLOX_VM

#include "vm/chunk.h"
#include "vm/value.h"

#define STACK_MAX 256

typedef struct vm {
    Chunk* chunk;
    Byte* ip;
    Value stack[STACK_MAX];
    Value* stackTop;
} VM;

typedef enum vm_interpret_result {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} VmInterpretResult;

void vm_init();
void vm_free();
VmInterpretResult vm_interpret(const char* code);

#endif
