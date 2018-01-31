#ifndef MYMEM_H
#define MYMEM_H
#include <stdlib.h>

void* alloc(size_t size);
void fr(void* mem);
void* clone(void* src, size_t size);
#endif