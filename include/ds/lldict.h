#ifndef LLDICT_H
#define LLDICT_H
#include "list.h"
#include <stdio.h>

#define DICT_INITIAL_CAPACITY 11

typedef struct key_value_pair_t {
    char* key;
    void* value;
    struct key_value_pair_t* next;
} KeyValuePair;

typedef int (*DictAction)(KeyValuePair* pair);

typedef struct linked_list_dict_t {
    KeyValuePair* buckets[DICT_INITIAL_CAPACITY];
    int capacity;
    int count;
    DictAction DeleteValue;
} LLDictionary;

LLDictionary* lldict(DictAction deleteValue);

int lldict_add(LLDictionary* dict, const char* key, void* value);
int lldict_remove(LLDictionary* dict, const char* key);
void* lldict_get(LLDictionary* dict, const char* key);
void lldict_destroy(LLDictionary* dict);
int lldict_contains(LLDictionary* dict, const char* key);
int lldict_set(LLDictionary* dict, const char* key, void* value);
#endif
