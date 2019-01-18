#ifndef CLOX_CHUNK
#define CLOX_CHUNK

#include "common.h"
#include "mem.h"
#include "vm/value.h"
#include <stdlib.h>

typedef enum clox_opcode {
    OP_RETURN,
    OP_CONSTANT,
} OpCode;

typedef struct clox_chunk {
    int capacity;
    int count;
    Byte* code;
    ValueArray constants;
    int* lines;
} Chunk;

void chunk_init(Chunk* chunk);

void chunk_write(Chunk* chunk, Byte value, int line);

void chunk_free(Chunk* chunk);

int chunk_constants_add(Chunk* chunk, Value value);

#endif
