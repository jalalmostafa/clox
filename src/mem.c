#include "except.h"
#include <stdlib.h>
#include <string.h>

void* alloc(size_t size)
{
    void* mem = malloc(size);
    if (mem == NULL) {
        except("No More Memory to allocate");
    }
    return mem;
}

void fr(void* mem)
{
    free(mem);
}

void* clone(void* src, size_t size)
{
    void* dst = alloc(size);
    memcpy(dst, src, size);
    return dst;
}
