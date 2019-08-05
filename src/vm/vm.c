#include "vm/vm.h"
#include "vm/compiler.h"
#include "vm/debug.h"
#include "vm/table.h"
#include "vm/value.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static void runtime_error(const char* format, ...);
static VmBoolean is_falsey(Value value);

VM vm;

static void vm_stack_reset()
{
    vm.stackTop = vm.stack;
    vm.frameCount = 0;
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
    VmFunction* function = NULL;
    CallFrame* frame = NULL;
    size_t instruction;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);
    for (int i = vm.frameCount - 1; i >= 0; i--) {
        frame = &vm.frames[i];
        function = frame->function;
        // -1 because the IP is sitting on the next instruction to be
        // executed.
        instruction = frame->ip - function->chunk.code - 1;
        fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);
        if (function->name == NULL) {
            fprintf(stderr, "script\n");
        } else {
            fprintf(stderr, "%s()\n", function->name->chars);
        }
    }
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
    size_t length = a->length + b->length;
    char* chars = (char*)alloc(length + 1);

    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = 0;

    result = vmstring_take(chars, length);
    vm_stack_push(object_val((VmObject*)result));
}

static int call(VmFunction* function, int argCount)
{
    if (argCount != function->arity) {
        runtime_error("Expected %d arguments but got %d.", function->arity, argCount);
        return 0;
    }

    if (vm.frameCount == FRAMES_MAX) {
        runtime_error("Stack overflow.");
        return 0;
    }

    CallFrame* frame = &vm.frames[vm.frameCount++];
    frame->function = function;
    frame->ip = function->chunk.code;

    frame->slots = vm.stackTop - argCount - 1;
    return 1;
}

static void native_define(const char* name, NativeFn function)
{
    vm_stack_push(object_val((VmObject*)vmstring_copy(name, (int)strlen(name))));
    vm_stack_push(object_val((VmObject*)vmnative_new(function)));
    table_set(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
    vm_stack_pop();
    vm_stack_pop();
}

int value_call(Value callable, int argCount)
{
    NativeFn native;
    Value result;

    if (IS_OBJECT(callable)) {
        switch (OBJECT_TYPE(callable)) {
        case OBJECT_FUNCTION:
            return call(AS_FUNCTION(callable), argCount);

        case OBJECT_NATIVE:
            native = AS_NATIVE(callable);
            result = native(argCount, vm.stackTop - argCount);
            vm.stackTop -= argCount + 1;
            vm_stack_push(result);
            return 1;
        default:
            // Non-callable object type.
            break;
        }
    }

    runtime_error("Can only call functions and classes.");
    return 0;
}

static Value native_clock(int argCount, Value* args)
{
    return number_val((double)clock() / CLOCKS_PER_SEC);
}

static VmInterpretResult vm_run()
{
    CallFrame* frame = &vm.frames[vm.frameCount - 1];
#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() (frame->ip += 2, (Short)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_CONSTANT() (frame->function->chunk.constants.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
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

    Short offset;
    Byte instruction, argCount;
    Value arbitraryValue, leftValue, rightValue, *slot = NULL;
    VmNumber left, right;
    VmString* name = NULL;

    for (;;) {
#ifdef DEBUG_EXECUTION_TRACE
        printf("    ");
        for (slot = vm.stack; slot < vm.stackTop; slot++) {
            printf("[ ");
            value_print(*slot);
            printf(" ]");
        }
        printf("\n");
        chunk_disassemble_instruction(&frame->function->chunk, (int)(frame->ip - frame->function->chunk.code));
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

            vm.frameCount--;
            if (vm.frameCount == 0) {
                return INTERPRET_OK;
            }

            vm.stackTop = frame->slots;
            vm_stack_push(arbitraryValue);

            frame = &vm.frames[vm.frameCount - 1];
            break;
        case OP_PRINT:
            arbitraryValue = vm_stack_pop();
            value_print(arbitraryValue);
            printf("\n");
            break;
        case OP_POP:
            vm_stack_pop();
            break;
        case OP_DEFINE_GLOBAL:
            name = READ_STRING();
            arbitraryValue = vm_stack_peek(0);
            table_set(&vm.globals, name, arbitraryValue);
            vm_stack_pop();
            break;
        case OP_GET_GLOBAL:
            name = READ_STRING();
            if (!table_get(&vm.globals, name, &arbitraryValue)) {
                runtime_error("Undefined variable at '%s'", name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
            vm_stack_push(arbitraryValue);
            break;
        case OP_SET_GLOBAL:
            name = READ_STRING();
            if (table_set(&vm.globals, name, vm_stack_peek(0))) {
                runtime_error("Undefined variable '%s'.", name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        case OP_GET_LOCAL:
            instruction = READ_BYTE();
            vm_stack_push(frame->slots[instruction]);
            break;
        case OP_SET_LOCAL:
            instruction = READ_BYTE();
            frame->slots[instruction] = vm_stack_peek(0);
            break;
        case OP_JUMP_IF_FALSE:
            offset = READ_SHORT();
            if (is_falsey(vm_stack_peek(0))) {
                frame->ip += offset;
            }
            break;
        case OP_JUMP:
            offset = READ_SHORT();
            frame->ip += offset;
            break;
        case OP_LOOP:
            offset = READ_SHORT();
            frame->ip -= offset;
            break;
        case OP_CALL:
            argCount = READ_BYTE();
            if (!value_call(vm_stack_peek(argCount), argCount)) {
                return INTERPRET_RUNTIME_ERROR;
            }
            frame = &vm.frames[vm.frameCount - 1];
            break;
        default:
            return INTERPRET_COMPILE_ERROR;
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_SHORT
#undef BINARY_OP
}

void vm_init()
{
    vm.objects = NULL;
    vm_stack_reset();
    table_init(&vm.strings);
    table_init(&vm.globals);
    native_define("clock", native_clock);
}

void vm_free()
{
    vm_stack_reset();
    objects_free();
    table_free(&vm.strings);
    table_free(&vm.globals);
}

VmInterpretResult vm_interpret(const char* code)
{
    VmFunction* function = compile(code);
    CallFrame* frame = NULL;

    if (function == NULL) {
        return INTERPRET_COMPILE_ERROR;
    }

    vm_stack_push(object_val((VmObject*)function));
    value_call(object_val((VmObject*)function), 0);

    return vm_run();
}
