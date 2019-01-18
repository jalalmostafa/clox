#include "vm/chunk.h"
#include "vm/value.h"

void chunk_init(Chunk* chunk)
{
    chunk->capacity = 0;
    chunk->count = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    value_array_init(&chunk->constants);
}

void chunk_write(Chunk* chunk, Byte value, int line)
{
    int oldCapacity = chunk->capacity;
    if (chunk->capacity < chunk->count + 1) {
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(chunk->code, Byte, oldCapacity, chunk->capacity);
        chunk->lines = GROW_ARRAY(chunk->lines, int, oldCapacity, chunk->capacity);
    }
    chunk->code[chunk->count] = value;
    chunk->lines[chunk->count] = line;
    chunk->count++;
}

void chunk_free(Chunk* chunk)
{
    FREE_ARRAY(Byte, chunk->code, chunk->capacity);
    FREE_ARRAY(int, chunk->lines, chunk->capacity);
    value_array_free(&chunk->constants);
    chunk_init(chunk);
}

int chunk_constants_add(Chunk* chunk, Value value)
{
    value_array_write(&chunk->constants, value);
    return chunk->constants.count - 1;
}
