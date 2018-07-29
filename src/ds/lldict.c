#include "ds/lldict.h"
#include "ds/list.h"
#include "mem.h"
#include <string.h>

static unsigned int hash_code(const char* key);
static KeyValuePair* lldict_get_bucket(LLDictionary* dict, const char* key);

LLDictionary* lldict(DictAction deleteValue)
{
    LLDictionary* dict = (LLDictionary*)alloc(sizeof(LLDictionary));
    memset(dict, 0, sizeof(LLDictionary));
    dict->capacity = DICT_INITIAL_CAPACITY;
    dict->count = 0;
    dict->DeleteValue = deleteValue;
    return dict;
}

int lldict_add(LLDictionary* dict, const char* key, void* value)
{
    KeyValuePair *pair = NULL, *bucket = NULL;
    int hash = 0;
    if (dict != NULL) {
        if (!lldict_contains(dict, key)) {
            pair = (KeyValuePair*)alloc(sizeof(KeyValuePair));
            pair->key = (char*)clone((void*)key, strlen(key) + 1);
            pair->value = value;
            pair->next = NULL;
            hash = hash_code(key);
            bucket = dict->buckets[hash];
            if (bucket == NULL) {
                dict->buckets[hash] = pair;
            } else {
                pair->next = dict->buckets[hash];
                dict->buckets[hash] = pair;
            }
            dict->count++;
            return 1;
        }
    }
    return 0;
}

int lldict_remove(LLDictionary* dict, const char* key)
{
    KeyValuePair *bucket = NULL, *prevBucket = NULL;
    int hash = hash_code(key);
    if (dict == NULL) {
        return 0;
    }

    bucket = dict->buckets[hash];

    if (bucket != NULL) {
        for (bucket = dict->buckets[hash]; bucket != NULL; bucket = bucket->next) {
            if (strcmp(key, bucket->key) == 0) {
                dict->DeleteValue(bucket);
                fr(bucket->key);
                prevBucket->next = bucket->next;
                fr(bucket);
                return 1;
            }
            prevBucket = bucket;
        }
    }

    return 0;
}

int lldict_contains(LLDictionary* dict, const char* key)
{
    return lldict_get_bucket(dict, key) != NULL;
}

void lldict_destroy(LLDictionary* dict)
{
    KeyValuePair *bucket = NULL, *nextBucket = NULL;
    int i = 0;
    if (dict != NULL) {
        for (i = 0; i < DICT_INITIAL_CAPACITY; i++) {
            nextBucket = bucket = dict->buckets[i];
            while (bucket != NULL) {
                dict->DeleteValue(bucket);
                fr(bucket->key);
                nextBucket = bucket->next;
                fr(bucket);
                bucket = nextBucket;
            }
        }
        fr(dict);
    }
}

static KeyValuePair* lldict_get_bucket(LLDictionary* dict, const char* key)
{
    KeyValuePair* bucket = NULL;
    int hash = 0;
    if (dict == NULL || dict->count == 0) {
        return NULL;
    }
    hash = hash_code(key);
    for (bucket = dict->buckets[hash]; bucket != NULL; bucket = bucket->next) {
        if (strcmp(key, bucket->key) == 0) {
            return bucket;
        }
    }
    return NULL;
}

void* lldict_get(LLDictionary* dict, const char* key)
{
    KeyValuePair* bucket = lldict_get_bucket(dict, key);
    return bucket != NULL ? bucket->value : NULL;
}

int lldict_set(LLDictionary* dict, const char* key, void* value)
{
    KeyValuePair* pair = NULL;
    if (dict != NULL) {
        pair = lldict_get(dict, key);
        if (pair != NULL) {
            pair->value = value;
            return 1;
        }
    }
    return 0;
}

static unsigned int hash_code(const char* key)
{
    return 0;
}