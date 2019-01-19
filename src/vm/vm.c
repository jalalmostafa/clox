#include "vm/vm.h"
#include "vm/debug.h"
#include <stdio.h>

VM vm;

static void vm_stack_reset()
{
    vm.stackTop = vm.stack;
}

static void vm_stack_push(Value value)
{
    *vm.stackTop = value;
    vm.stackTop++;
}

static Value vm_stack_pop()
{
    return *--vm.stackTop;
}

static VmInterpretResult run()
{
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define BINARY_OP(op)                 \
    do {                              \
        right = vm_stack_pop();       \
        left = vm_stack_pop();        \
        vm_stack_push(left op right); \
    } while (0)

    Byte instruction;
    Value arbitraryValue, *slot = NULL;
    double left, right;

    for (;;) {
#ifdef DEBUG_EXECUTION_TRACE
        printf("\t\t");
        for (slot = vm.stack; slot < vm.stackTop; slot++) {
            printf("[ ");
            value_print(*slot);
            printf(" ]");
        }
        printf("\n");
        chunk_disassemble_instruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
        switch (instruction = READ_BYTE()) {
        case OP_CONSTANT:
            arbitraryValue = READ_CONSTANT();
            vm_stack_push(arbitraryValue);
            value_print(arbitraryValue);
            printf("\n");
            break;
        case OP_NEGATE:
            arbitraryValue = vm_stack_pop();
            vm_stack_push(-arbitraryValue);
            break;
        case OP_ADD:
            BINARY_OP(+);
            break;
        case OP_SUBTRACT:
            BINARY_OP(-);
            break;
        case OP_MULTIPLY:
            BINARY_OP(*);
            break;
        case OP_DIVIDE:
            BINARY_OP(/);
            break;
        case OP_RETURN:
            arbitraryValue = vm_stack_pop();
            value_print(arbitraryValue);
            printf("\n");
            return INTERPRET_OK;
        default:
            return INTERPRET_COMPILE_ERROR;
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

void vm_init()
{
    vm.chunk = NULL;
    vm.ip = NULL;
    vm_stack_reset();
}

void vm_free()
{
    vm.chunk = NULL;
    vm.ip = NULL;
    vm_stack_reset();
}

VmInterpretResult vm_interpret(Chunk* chunk)
{
    vm.chunk = chunk;
    vm.ip = vm.chunk->code;
    return run();
}
