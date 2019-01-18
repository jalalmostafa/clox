#include "vm/vm.h"
#include "vm/debug.h"
#include <stdio.h>

static VmInterpretResult run();

VM vm;

void vm_init()
{
    vm.chunk = NULL;
    vm.ip = NULL;
}

void vm_free()
{
}

VmInterpretResult vm_interpret(Chunk* chunk)
{
    vm.chunk = chunk;
    vm.ip = vm.chunk->code;
    return run();
}

static VmInterpretResult run()
{
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
    Byte instruction;
    Value constant;
    for (;;) {
#ifdef DEBUG_EXECUTION_TRACE
        chunk_disassemble_instruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
        switch (instruction = READ_BYTE()) {
        case OP_CONSTANT:
            constant = READ_CONSTANT();
            value_print(constant);
            printf("\n");
            break;
        case OP_RETURN:
            return INTERPRET_OK;
        default:
            return INTERPRET_COMPILE_ERROR;
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
}