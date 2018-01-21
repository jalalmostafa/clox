#include "except.h"
#include <stdlib.h>

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
    if (mem != NULL) {
        free(mem);
    }
}