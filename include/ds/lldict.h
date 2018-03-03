#ifndef LLDICT_H
#define LLDICT_H
#include "list.h"
#include <stdio.h>

typedef struct key_value_pair_t {
    char* key;
    void* value;
} KeyValuePair;

typedef struct linked_list_dict_t {
    List* elements;
} LLDictionary;

LLDictionary* lldict();

int lldict_add(LLDictionary* dict, const char* key, void* value);
int lldict_remove(LLDictionary* dict, const char* key);
void* lldict_get(LLDictionary* dict, const char* key);
void lldict_destroy(LLDictionary* dict);
KeyValuePair* lldict_contains(LLDictionary* dict, const char* key);
int lldict_set(LLDictionary* dict, const char* key, void* value);
#endif