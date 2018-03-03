#ifndef LLDICT_H
#define LLDICT_H
#include "list.h"

typedef struct key_value_pair_t {
    char* key;
    void* value;
} KeyValuePair;

typedef struct linked_list_dict_t {
    List* elements;
} LLDictionary;

LLDictionary* lldict();

void lldict_add(LLDictionary* dict, const char* key, void* value, size_t valueSize);
int lldict_remove(LLDictionary* dict, const char* key);
void* lldict_get(LLDictionary* dict, const char* key);
void lldict_destroy(LLDictionary* dict);
#endif