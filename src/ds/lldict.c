#include "ds/lldict.h"
#include "ds/list.h"
#include "mem.h"
#include <string.h>

static void keyvaluepair_delete(void* keyValuePairObj);

LLDictionary* lldict()
{
    LLDictionary* dict = (LLDictionary*)alloc(sizeof(LLDictionary));
    dict->elements = list();
    return dict;
}

int lldict_add(LLDictionary* dict, const char* key, void* value)
{
    KeyValuePair* pair = NULL;
    if (dict != NULL) {
        if (!lldict_contains(dict, key)) {
            pair = (KeyValuePair*)alloc(sizeof(KeyValuePair));
            pair->key = (char*)clone((void*)key, strlen(key) + 1);
            pair->value = value;
            return list_push(dict->elements, pair) != NULL;
        }
    }
    return 0;
}

int lldict_remove(LLDictionary* dict, const char* key)
{
    Node* node = NULL;
    Node* removed = NULL;
    KeyValuePair* pair = NULL;
    int keyLength = 0;
    if (dict == NULL) {
        return 0;
    }
    keyLength = strlen(key) + 1;
    for (node = dict->elements->head; node != NULL; node = node->next) {
        pair = (KeyValuePair*)node->data;
        if (memcmp(pair->key, key, keyLength) == 0) {
            removed = node;
            keyvaluepair_delete(pair);
            break;
        }
    }
    if (removed == NULL) {
        return 0;
    }
    return list_remove(dict->elements, removed);
}

KeyValuePair* lldict_contains(LLDictionary* dict, const char* key)
{
    Node *node = NULL;
    int keyLength = strlen(key) + 1;
    if (dict != NULL) {
        for (node = dict->elements->head; node != NULL; node = node->next) {
            if (memcmp(((KeyValuePair*)node->data)->key, key, keyLength) == 0) {
                return (KeyValuePair*)node->data;
            }
        }
    }
    return NULL;
}

void lldict_destroy(LLDictionary* dict)
{
    if (dict != NULL) {
        list_foreach(dict->elements, keyvaluepair_delete);
        list_destroy(dict->elements);
        fr(dict);
    }
}

void* lldict_get(LLDictionary* dict, const char* key)
{
    Node* node = NULL;
    KeyValuePair* pair = NULL;
    int keyLength = 0;
    if (dict == NULL) {
        return 0;
    }
    keyLength = strlen(key) + 1;
    for (node = dict->elements->head; node != NULL; node = node->next) {
        pair = (KeyValuePair*)node->data;
        if (memcmp(pair->key, key, keyLength) == 0) {
            return pair->value;
        }
    }
    return NULL;
}

int lldict_set(LLDictionary* dict, const char* key, void* value)
{
    KeyValuePair* pair = NULL;
    if (dict != NULL) {
        pair = lldict_contains(dict, key);
        if (pair != NULL) {
            pair->value = value;
            return 1;
        }
    }
    return 0;
}

static void keyvaluepair_delete(void* keyValuePairObj)
{
    KeyValuePair* keyValuePair = (KeyValuePair*)keyValuePairObj;
    if (keyValuePair != NULL) {
        fr(keyValuePair->key);
        fr(keyValuePair);
    }
}
