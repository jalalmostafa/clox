#include "vm/vm.h"
#include "vm/compiler.h"
#include "vm/debug.h"
#include "vm/table.h"
#include "vm/value.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static void runtime_error(const char* format, ...);
static VmBoolean is_falsey(Value value);

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

static Value vm_stack_peek(int distance)
{
    return vm.stackTop[-1 - distance];
}

static void runtime_error(const char* format, ...)
{
    va_list args;
    size_t instruction;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);
    instruction = vm.ip - vm.chunk->code;
    fprintf(stderr, "[line %d] in script\n", vm.chunk->lines[instruction]);
    vm_stack_reset();
}

static VmBoolean is_falsey(Value value)
{
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void vmstring_concatenate()
{
    VmString* b = AS_STRING(vm_stack_pop());
    VmString* a = AS_STRING(vm_stack_pop());
    VmString* result = NULL;
    int length = a->length + b->length;
    char* chars = (char*)alloc(length + 1);

    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = 0;

    result = vmstring_take(chars, length);
    vm_stack_push(object_val((VmObject*)result));
}

static VmInterpretResult vm_run()
{
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define BINARY_OP(valueType, op)                                            \
    do {                                                                    \
        if (!IS_NUMBER(vm_stack_peek(0)) || !IS_NUMBER(vm_stack_peek(1))) { \
            runtime_error("Operands must be numbers.");                     \
            return INTERPRET_RUNTIME_ERROR;                                 \
        }                                                                   \
        right = AS_NUMBER(vm_stack_pop());                                  \
        left = AS_NUMBER(vm_stack_pop());                                   \
        vm_stack_push(valueType(left op right));                            \
    } while (0)

    Byte instruction;
    Value arbitraryValue, leftValue, rightValue, *slot = NULL;
    VmNumber left, right;

    for (;;) {
#ifdef DEBUG_EXECUTION_TRACE
        printf("    ");
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
        case OP_NOT:
            arbitraryValue = vm_stack_pop();
            vm_stack_push(bool_val(is_falsey(arbitraryValue)));
            break;
        case OP_NEGATE:
            if (!IS_NUMBER(vm_stack_peek(0))) {
                runtime_error("OPerand must be a number");
                return INTERPRET_RUNTIME_ERROR;
            }
            arbitraryValue = vm_stack_pop();
            vm_stack_push(number_val(-AS_NUMBER(arbitraryValue)));
            break;
        case OP_ADD:
            if (IS_STRING(vm_stack_peek(0)) && IS_STRING(vm_stack_peek(1))) {
                vmstring_concatenate();
            } else if (IS_NUMBER(vm_stack_peek(0)) && IS_NUMBER(vm_stack_peek(1))) {
                right = AS_NUMBER(vm_stack_pop());
                left = AS_NUMBER(vm_stack_pop());
                vm_stack_push(number_val(left + right));
            } else {
                runtime_error("Operands must be two numbers or two strings.");
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        case OP_SUBTRACT:
            BINARY_OP(number_val, -);
            break;
        case OP_MULTIPLY:
            BINARY_OP(number_val, *);
            break;
        case OP_DIVIDE:
            BINARY_OP(number_val, /);
            break;
        case OP_NIL:
            vm_stack_push(nil_val());
            break;
        case OP_TRUE:
            vm_stack_push(bool_val(1));
            break;
        case OP_FALSE:
            vm_stack_push(bool_val(0));
            break;
        case OP_EQUAL:
            rightValue = vm_stack_pop();
            leftValue = vm_stack_pop();
            vm_stack_push(bool_val(values_equal(leftValue, rightValue)));
            break;
        case OP_GREATER:
            BINARY_OP(bool_val, >);
            break;
        case OP_LESS:
            BINARY_OP(bool_val, <);
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
    vm.objects = NULL;
    vm_stack_reset();
    table_init(&vm.strings);
}

void vm_free()
{
    vm.chunk = NULL;
    vm.ip = NULL;
    vm_stack_reset();
    objects_free();
    table_free(&vm.strings);
}

VmInterpretResult vm_interpret(const char* code)
{
    Chunk chunk;
    VmInterpretResult result;

    chunk_init(&chunk);
    if (!compile(code, &chunk)) {
        chunk_free(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    vm_init();
    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    result = vm_run();

    chunk_free(&chunk);
    vm_free();
    return result;
}
