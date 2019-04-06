#include "vm/debug.h"
#include <stdio.h>

static int instruction_simple(const char* name, int offset);
static int instruction_constant(const char* name, Chunk* chunk, int offset);

void chunk_disassemble(Chunk* chunk, const char* name)
{
    int offset;
    printf("== %s ==\n", name);
    for (offset = 0; offset < chunk->count;) {
        offset = chunk_disassemble_instruction(chunk, offset);
    }
    printf("==============\n");
}

int chunk_disassemble_instruction(Chunk* chunk, int offset)
{
    Byte instruction;
    printf("%04d ", offset);
    if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1]) {
        printf("   | ");
    } else {
        printf("%4d ", chunk->lines[offset]);
    }
    instruction = chunk->code[offset];
    switch (instruction) {
    case OP_RETURN:
        return instruction_simple("OP_RETURN", offset);
    case OP_CONSTANT:
        return instruction_constant("OP_CONSTANT", chunk, offset);
    case OP_NEGATE:
        return instruction_simple("OP_NEGATE", offset);
    case OP_ADD:
        return instruction_simple("OP_ADD", offset);
    case OP_SUBTRACT:
        return instruction_simple("OP_SUBTRACT", offset);
    case OP_MULTIPLY:
        return instruction_simple("OP_MULTIPLY", offset);
    case OP_DIVIDE:
        return instruction_simple("OP_DIVIDE", offset);
    case OP_NIL:
        return instruction_simple("OP_NIL", offset);
    case OP_TRUE:
        return instruction_simple("OP_TRUE", offset);
    case OP_FALSE:
        return instruction_simple("OP_FALSE", offset);
    case OP_NOT:
        return instruction_simple("OP_NOT", offset);
    case OP_EQUAL:
        return instruction_simple("OP_EQUAL", offset);
    case OP_GREATER:
        return instruction_simple("OP_GREATER", offset);
    case OP_LESS:
        return instruction_simple("OP_LESS", offset);
    case OP_PRINT:
        return instruction_simple("OP_PRINT", offset);
    case OP_POP:
        return instruction_simple("OP_POP", offset);
    case OP_DEFINE_GLOBAL:
        return instruction_constant("OP_DEFINE_GLOBAL", chunk, offset);
    case OP_GET_GLOBAL:
        return instruction_constant("OP_GET_GLOBAL", chunk, offset);
    case OP_SET_GLOBAL:
        return instruction_constant("OP_SET_GLOBAL", chunk, offset);
    default:
        printf("Unknow opcode %d\n", instruction);
        return offset + 1;
    }
}

static int instruction_simple(const char* name, int offset)
{
    printf("%s\n", name);
    return offset + 1;
}

static int instruction_constant(const char* name, Chunk* chunk, int offset)
{
    Byte constant = chunk->code[offset + 1];
    printf("%-16s %4d '", name, constant);
    value_print(chunk->constants.values[constant]);
    printf("'\n");
    return offset + 2;
}
