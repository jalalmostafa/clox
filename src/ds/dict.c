#include "ds/dict.h"
#include "ds/list.h"
#include "mem.h"
#include <string.h>

static unsigned int hash_code(const char* key);
static KeyValuePair* dict_get_bucket(Dictionary* dict, const char* key);

Dictionary* dict(DictAction deleteValue)
{
    Dictionary* dict = (Dictionary*)alloc(sizeof(Dictionary));
    memset(dict, 0, sizeof(Dictionary));
    dict->capacity = DICT_INITIAL_CAPACITY;
    dict->count = 0;
    dict->DeleteValue = deleteValue;
    return dict;
}

int dict_add(Dictionary* dict, const char* key, void* value)
{
    KeyValuePair *pair = NULL, *bucket = NULL;
    int hash = 0;
    if (dict != NULL) {
        if (!dict_contains(dict, key)) {
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

int dict_remove(Dictionary* dict, const char* key)
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

int dict_contains(Dictionary* dict, const char* key)
{
    return dict_get_bucket(dict, key) != NULL;
}

void dict_destroy(Dictionary* dict)
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

static KeyValuePair* dict_get_bucket(Dictionary* dict, const char* key)
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

void* dict_get(Dictionary* dict, const char* key)
{
    KeyValuePair* bucket = dict_get_bucket(dict, key);
    return bucket != NULL ? bucket->value : NULL;
}

int dict_set(Dictionary* dict, const char* key, void* value)
{
    KeyValuePair* pair = NULL;
    if (dict != NULL) {
        pair = dict_get(dict, key);
        if (pair != NULL) {
            pair->value = value;
            return 1;
        }
    }
    return 0;
}

static unsigned int hash_code(const char* key)
{
    unsigned int hash = 0, i = 0;
    size_t length = strlen(key);
    for (i = 0; i < length; i++) {
        hash = 31 * hash + key[i];
    }
    return hash % DICT_INITIAL_CAPACITY;
}
